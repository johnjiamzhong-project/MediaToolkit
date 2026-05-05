#include "AudioWorker.h"
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
}

AudioWorker::AudioWorker(QObject *parent) : QObject(parent) {}

void AudioWorker::cancel() { m_cancel = true; }

void AudioWorker::doExtract(const QString &inputPath, const QString &outputPath, const QString &format)
{
    m_cancel = false;

    // ── 1. 打开输入文件 ──
    AVFormatContext *inFmt = nullptr;
    // avformat_open_input — 打开文件并探测容器格式，成功返回 0，失败返回负数
    if (avformat_open_input(&inFmt, inputPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开输入文件"); return;
    }
    // avformat_find_stream_info — 解码少量数据包以填充流参数，对无头部信息的格式尤为必要
    if (avformat_find_stream_info(inFmt, nullptr) < 0) {
        avformat_close_input(&inFmt);
        emit errorOccurred("无法读取流信息"); return;
    }
    // av_find_best_stream — 在所有流中找出最佳音频流索引，失败返回负数
    int audioIdx = av_find_best_stream(inFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioIdx < 0) {
        avformat_close_input(&inFmt);
        emit errorOccurred("未找到音频流"); return;
    }

    // ── 2. 打开解码器 ──
    AVCodecParameters *inPar = inFmt->streams[audioIdx]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(inPar->codec_id);
    if (!decoder) {
        avformat_close_input(&inFmt);
        emit errorOccurred("找不到音频解码器"); return;
    }
    AVCodecContext *decCtx = avcodec_alloc_context3(decoder);
    // avcodec_parameters_to_context — 将流的 AVCodecParameters 拷贝到解码器上下文（含 ch_layout、sample_rate 等）
    avcodec_parameters_to_context(decCtx, inPar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法打开音频解码器"); return;
    }
    // 部分解码器未设置 ch_layout，回退到默认立体声
    if (decCtx->ch_layout.nb_channels == 0)
        av_channel_layout_default(&decCtx->ch_layout, 2);

    // ── 3. 查找编码器 ──
    // MP3 使用 libmp3lame（AV_CODEC_ID_MP3），WAV 使用 PCM 小端 16-bit（AV_CODEC_ID_PCM_S16LE）
    AVCodecID encId = (format == "mp3") ? AV_CODEC_ID_MP3 : AV_CODEC_ID_PCM_S16LE;
    const AVCodec *encoder = avcodec_find_encoder(encId);
    if (!encoder) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred(QString("找不到 %1 编码器（FFmpeg 可能未包含 libmp3lame）").arg(format.toUpper()));
        return;
    }

    // ── 4. 配置编码器上下文 ──
    AVCodecContext *encCtx = avcodec_alloc_context3(encoder);
    encCtx->sample_rate = decCtx->sample_rate > 0 ? decCtx->sample_rate : 44100;
    // encoder->sample_fmts 列出编码器支持的采样格式（优先使用第一个）
    // libmp3lame 要求 AV_SAMPLE_FMT_FLTP；PCM 使用 AV_SAMPLE_FMT_S16
    encCtx->sample_fmt = encoder->sample_fmts ? encoder->sample_fmts[0] : AV_SAMPLE_FMT_S16;
    // av_channel_layout_copy — 深拷贝 ch_layout（自定义通道顺序时会分配堆内存）
    av_channel_layout_copy(&encCtx->ch_layout, &decCtx->ch_layout);
    if (format == "mp3") encCtx->bit_rate = 128000;

    // ── 5. 创建输出格式上下文 ──
    AVFormatContext *outFmt = nullptr;
    // avformat_alloc_output_context2 — 按格式名称（"mp3"/"wav"）自动选择复用器
    if (avformat_alloc_output_context2(&outFmt, nullptr,
            format.toUtf8().constData(),
            outputPath.toUtf8().constData()) < 0) {
        avcodec_free_context(&encCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法创建输出格式上下文"); return;
    }
    // AVFMT_GLOBALHEADER：某些格式（如 AAC）需要将编解码器头部写入容器全局区域
    if (outFmt->oformat->flags & AVFMT_GLOBALHEADER)
        encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(encCtx, encoder, nullptr) < 0) {
        avformat_free_context(outFmt);
        avcodec_free_context(&encCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法打开音频编码器"); return;
    }

    // ── 6. 创建输出音频流 ──
    AVStream *outStream = avformat_new_stream(outFmt, nullptr);
    // avcodec_parameters_from_context — 将编码器参数写入输出流的 codecpar（供复用器使用）
    avcodec_parameters_from_context(outStream->codecpar, encCtx);
    outStream->time_base = {1, encCtx->sample_rate};

    // ── 7. 打开输出文件 & 写文件头 ──
    // AVFMT_NOFILE：某些"格式"（如 image2）不需要真实文件，mp3/wav 都需要
    if (!(outFmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFmt->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
            avformat_free_context(outFmt);
            avcodec_free_context(&encCtx);
            avcodec_free_context(&decCtx);
            avformat_close_input(&inFmt);
            emit errorOccurred("无法打开输出文件"); return;
        }
    }
    // avformat_write_header — 向输出文件写入容器头部，可能调整 outStream->time_base
    if (avformat_write_header(outFmt, nullptr) < 0) {
        avio_closep(&outFmt->pb);
        avformat_free_context(outFmt);
        avcodec_free_context(&encCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法写入文件头"); return;
    }

    // ── 8. 初始化重采样器 ──
    // SwrContext 负责：采样格式转换（如 FLTP→S16）、采样率转换、声道布局转换
    SwrContext *swr = nullptr;
    // swr_alloc_set_opts2 — FFmpeg 6.1+ 新 API，使用 AVChannelLayout 代替旧版 channel_layout(uint64_t)
    if (swr_alloc_set_opts2(&swr,
            &encCtx->ch_layout, encCtx->sample_fmt, encCtx->sample_rate,
            &decCtx->ch_layout, decCtx->sample_fmt, decCtx->sample_rate,
            0, nullptr) < 0 || swr_init(swr) < 0) {
        if (swr) swr_free(&swr);
        av_write_trailer(outFmt);
        avio_closep(&outFmt->pb);
        avformat_free_context(outFmt);
        avcodec_free_context(&encCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法初始化重采样器"); return;
    }

    // ── 9. 音频 FIFO ──
    // MP3 编码器（libmp3lame）每帧固定 1152 个采样，需要 FIFO 缓冲凑满一帧再编码
    // PCM 编码器支持可变帧大小（frame_size=0），用默认 4096 同样走 FIFO 逻辑保持统一
    int frameSize = encCtx->frame_size > 0 ? encCtx->frame_size : 4096;
    // av_audio_fifo_alloc(fmt, channels, initial_capacity) — 分配音频 FIFO，初始容量可自动扩展
    AVAudioFifo *fifo = av_audio_fifo_alloc(
        encCtx->sample_fmt, encCtx->ch_layout.nb_channels, frameSize * 4);

    // ── 辅助：从 FIFO 编码并写包 ──
    AVPacket *inPkt    = av_packet_alloc();
    AVFrame  *decFrame = av_frame_alloc();
    AVFrame  *encFrame = av_frame_alloc();
    int64_t   pts      = 0;  // 输出 PTS，单位：采样数（time_base = {1, sample_rate}）

    AVRational encTb = {1, encCtx->sample_rate};

    auto encodeFromFifo = [&](bool flush) {
        int minSamples = flush ? 1 : frameSize;
        while (av_audio_fifo_size(fifo) >= minSamples) {
            int toRead = std::min(av_audio_fifo_size(fifo), frameSize);
            // 设置编码帧参数后调用 av_frame_get_buffer 分配数据缓冲区
            encFrame->format      = encCtx->sample_fmt;
            encFrame->sample_rate = encCtx->sample_rate;
            encFrame->nb_samples  = toRead;
            av_channel_layout_copy(&encFrame->ch_layout, &encCtx->ch_layout);
            av_frame_get_buffer(encFrame, 0);
            // av_audio_fifo_read — 从 FIFO 读取 toRead 个采样到 encFrame->data
            av_audio_fifo_read(fifo, (void**)encFrame->data, toRead);
            encFrame->pts = pts;
            pts += toRead;

            // avcodec_send_frame — 向编码器送入原始帧，编码器内部持有引用
            avcodec_send_frame(encCtx, encFrame);
            // av_frame_unref — 释放我们持有的缓冲区引用（编码器自己的引用仍有效）
            av_frame_unref(encFrame);

            // avcodec_receive_packet — 取出已编码的压缩包，EAGAIN 表示需继续送帧
            AVPacket *outPkt = av_packet_alloc();
            while (avcodec_receive_packet(encCtx, outPkt) == 0) {
                // av_packet_rescale_ts — 将 pts/dts/duration 从编码器时间基转换到输出流时间基
                av_packet_rescale_ts(outPkt, encTb, outStream->time_base);
                outPkt->stream_index = 0;
                // av_interleaved_write_frame — 交错写入（内部缓冲确保 dts 单调递增）
                av_interleaved_write_frame(outFmt, outPkt);
                av_packet_unref(outPkt);
            }
            av_packet_free(&outPkt);
        }
    };

    // ── 10. 主解码-重采样-编码循环 ──
    double    durationSec  = inFmt->duration > 0 ? (double)inFmt->duration / AV_TIME_BASE : 0.0;
    int       lastProgress = -1;
    AVRational audioTb     = inFmt->streams[audioIdx]->time_base;

    while (!m_cancel && av_read_frame(inFmt, inPkt) >= 0) {
        if (inPkt->stream_index == audioIdx) {
            if (avcodec_send_packet(decCtx, inPkt) == 0) {
                while (avcodec_receive_frame(decCtx, decFrame) == 0) {
                    // 进度：用解码帧的 PTS 除以总时长估算百分比
                    if (durationSec > 0 && decFrame->pts != AV_NOPTS_VALUE) {
                        double cur = decFrame->pts * av_q2d(audioTb);
                        int p = std::min((int)(cur / durationSec * 100.0), 99);
                        if (p != lastProgress) { lastProgress = p; emit progressUpdated(p); }
                    }

                    // 计算重采样后的最大输出采样数
                    // swr_get_delay — 返回重采样器内部缓冲中尚未输出的采样数（输入采样率单位）
                    // av_rescale_rnd(a, b, c, AV_ROUND_UP) — 计算 a*b/c，向上取整
                    int outSamples = (int)av_rescale_rnd(
                        swr_get_delay(swr, decCtx->sample_rate) + decFrame->nb_samples,
                        encCtx->sample_rate, decCtx->sample_rate, AV_ROUND_UP);

                    AVFrame *swrFrame = av_frame_alloc();
                    swrFrame->format      = encCtx->sample_fmt;
                    swrFrame->sample_rate = encCtx->sample_rate;
                    swrFrame->nb_samples  = outSamples;
                    av_channel_layout_copy(&swrFrame->ch_layout, &encCtx->ch_layout);
                    av_frame_get_buffer(swrFrame, 0);

                    // swr_convert(swr, dst, dst_count, src, src_count)
                    //   执行重采样/格式转换，返回实际输出采样数；src=nullptr 时刷出内部缓存
                    int converted = swr_convert(swr, swrFrame->data, outSamples,
                        (const uint8_t**)decFrame->data, decFrame->nb_samples);
                    if (converted > 0)
                        av_audio_fifo_write(fifo, (void**)swrFrame->data, converted);
                    av_frame_free(&swrFrame);

                    encodeFromFifo(false);
                    av_frame_unref(decFrame);
                }
            }
        }
        av_packet_unref(inPkt);
    }

    // ── 11. 刷新：解码器尾帧 → 重采样剩余 → 编码器 ──
    if (!m_cancel) {
        // 向解码器发送 nullptr 通知进入刷新模式，取出所有缓冲帧
        avcodec_send_packet(decCtx, nullptr);
        while (avcodec_receive_frame(decCtx, decFrame) == 0) {
            int outSamples = (int)av_rescale_rnd(
                swr_get_delay(swr, decCtx->sample_rate) + decFrame->nb_samples,
                encCtx->sample_rate, decCtx->sample_rate, AV_ROUND_UP);
            AVFrame *swrFrame = av_frame_alloc();
            swrFrame->format      = encCtx->sample_fmt;
            swrFrame->sample_rate = encCtx->sample_rate;
            swrFrame->nb_samples  = outSamples;
            av_channel_layout_copy(&swrFrame->ch_layout, &encCtx->ch_layout);
            av_frame_get_buffer(swrFrame, 0);
            int converted = swr_convert(swr, swrFrame->data, outSamples,
                (const uint8_t**)decFrame->data, decFrame->nb_samples);
            if (converted > 0)
                av_audio_fifo_write(fifo, (void**)swrFrame->data, converted);
            av_frame_free(&swrFrame);
            encodeFromFifo(false);
            av_frame_unref(decFrame);
        }

        // 刷出 swr 内部残留的采样（src=nullptr, src_count=0）
        int delayedSamples = (int)av_rescale_rnd(
            swr_get_delay(swr, decCtx->sample_rate),
            encCtx->sample_rate, decCtx->sample_rate, AV_ROUND_UP);
        if (delayedSamples > 0) {
            AVFrame *swrFrame = av_frame_alloc();
            swrFrame->format      = encCtx->sample_fmt;
            swrFrame->sample_rate = encCtx->sample_rate;
            swrFrame->nb_samples  = delayedSamples;
            av_channel_layout_copy(&swrFrame->ch_layout, &encCtx->ch_layout);
            av_frame_get_buffer(swrFrame, 0);
            int converted = swr_convert(swr, swrFrame->data, delayedSamples, nullptr, 0);
            if (converted > 0)
                av_audio_fifo_write(fifo, (void**)swrFrame->data, converted);
            av_frame_free(&swrFrame);
        }

        // 编码 FIFO 中剩余的所有采样（flush=true，不足一帧也编码）
        encodeFromFifo(true);

        // 向编码器发送 nullptr，刷出编码器内部最后一批包
        avcodec_send_frame(encCtx, nullptr);
        AVPacket *outPkt = av_packet_alloc();
        while (avcodec_receive_packet(encCtx, outPkt) == 0) {
            av_packet_rescale_ts(outPkt, encTb, outStream->time_base);
            outPkt->stream_index = 0;
            av_interleaved_write_frame(outFmt, outPkt);
            av_packet_unref(outPkt);
        }
        av_packet_free(&outPkt);

        // av_write_trailer — 写入容器尾部（更新 WAV 头部中的数据块大小等）
        av_write_trailer(outFmt);
        emit progressUpdated(100);
    }

    // ── 12. 清理所有资源 ──
    av_audio_fifo_free(fifo);
    swr_free(&swr);
    av_frame_free(&encFrame);
    av_frame_free(&decFrame);
    av_packet_free(&inPkt);
    avcodec_free_context(&encCtx);
    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outFmt->pb);
    avformat_free_context(outFmt);
    avcodec_free_context(&decCtx);
    avformat_close_input(&inFmt);

    if (m_cancel)
        emit errorOccurred("已取消");
    else
        emit finished(outputPath);
}
