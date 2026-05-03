#include "ScreenshotWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

ScreenshotWorker::ScreenshotWorker(QObject *parent) : QObject(parent) {}

void ScreenshotWorker::probeVideo(const QString &filePath)
{
    AVFormatContext *fmt = nullptr;
    // avformat_open_input(ctx, url, fmt, options) — 打开文件，探测容器格式，填充 AVFormatContext
    //   成功返回 0，失败返回负数错误码
    if (avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit videoInfoReady(0, 0, 0, 0, 0.0); return;
    }
    // avformat_find_stream_info(ctx, options) — 解码少量包以填充各流编解码参数，成功返回 >= 0
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        emit videoInfoReady(0, 0, 0, 0, 0.0); return;
    }

    // av_find_best_stream(ctx, type, wanted, related, decoder, flags)
    //   在所有流中选出指定类型（此处 VIDEO）的最佳流索引。
    //   wanted_stream=-1 和 related_stream=-1 表示无偏好；decoder=nullptr 忽略解码器可用性。
    //   返回流索引（>=0），未找到返回负数错误码。
    int videoStream = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    double fps = 0.0;
    int width = 0, height = 0;
    qint64 totalFrames = 0;

    if (videoStream >= 0) {
        AVStream *st = fmt->streams[videoStream];
        // st->avg_frame_rate — 容器记录的平均帧率（AVRational 分数形式）
        // av_q2d(q) — 将 AVRational（num/den）转换为 double；den=0 时结果为 0
        if (st->avg_frame_rate.den > 0)
            fps = av_q2d(st->avg_frame_rate);
        width  = st->codecpar->width;
        height = st->codecpar->height;
        // st->nb_frames — 容器头部记录的总帧数，部分格式（如 MPEG-TS）中为 0
        if (st->nb_frames > 0)
            totalFrames = st->nb_frames;
        else if (fps > 0 && fmt->duration > 0)
            // 容器未记录帧数时，用 fps × 时长估算（AV_TIME_BASE = 1,000,000，即微秒）
            totalFrames = (qint64)(fps * fmt->duration / AV_TIME_BASE);
    }

    // fmt->duration — 总时长，单位 AV_TIME_BASE（微秒）；除以 AV_TIME_BASE 得秒
    double durationSec = fmt->duration > 0 ? (double)fmt->duration / AV_TIME_BASE : 0.0;
    // avformat_close_input(&ctx) — 关闭文件，释放 AVFormatContext 及所有内部资源，ctx 置 nullptr
    avformat_close_input(&fmt);

    emit videoInfoReady(durationSec, totalFrames, width, height, fps);
}

void ScreenshotWorker::doWork(const QString &filePath, double seconds)
{
    AVFormatContext *fmt = nullptr;
    // avformat_open_input — 打开文件，探测容器格式；成功返回 0，失败返回负数
    if (avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开文件"); return;
    }
    // avformat_find_stream_info — 探测流参数，对不在头部存储参数的格式（如 MPEG-TS）尤为必要
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        emit errorOccurred("无法读取流信息"); return;
    }

    // av_find_best_stream — 选出最佳视频流索引；失败（无视频流）返回负数
    int videoStream = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream < 0) {
        avformat_close_input(&fmt);
        emit errorOccurred("未找到视频流"); return;
    }

    AVCodecParameters *par = fmt->streams[videoStream]->codecpar;
    // avcodec_find_decoder(codec_id) — 按 codec_id 查找内置软解码器，返回只读 AVCodec 指针或 nullptr
    const AVCodec *codec   = avcodec_find_decoder(par->codec_id);
    // avcodec_alloc_context3(codec) — 分配并初始化 AVCodecContext，传入 codec 可预填默认参数
    AVCodecContext *ctx    = avcodec_alloc_context3(codec);
    // avcodec_parameters_to_context(ctx, par) — 将流的 AVCodecParameters 拷贝到解码器上下文
    //   必须在 avcodec_open2 前调用，否则解码器缺少宽高/像素格式等参数
    avcodec_parameters_to_context(ctx, par);
    // avcodec_open2(ctx, codec, options) — 用指定解码器初始化上下文，分配解码器内部状态
    //   成功返回 0，失败返回负数
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        // avcodec_free_context(&ctx) — 关闭解码器并释放 AVCodecContext，ctx 置 nullptr
        avcodec_free_context(&ctx);
        avformat_close_input(&fmt);
        emit errorOccurred("无法打开解码器"); return;
    }

    // av_seek_frame(ctx, stream_index, timestamp, flags)
    //   将读取位置定位到指定时间戳（stream_index=-1 表示使用默认时间基 AV_TIME_BASE）。
    //   AVSEEK_FLAG_BACKWARD：seek 到不晚于目标时间的最近关键帧（I-frame），确保可以解码。
    //   成功返回 >= 0，失败返回负数；失败时回退到文件开头，避免因容器索引缺失导致整体失败。
    int64_t seekTs = (int64_t)(seconds * AV_TIME_BASE);
    if (av_seek_frame(fmt, -1, seekTs, AVSEEK_FLAG_BACKWARD) < 0)
        av_seek_frame(fmt, -1, 0, AVSEEK_FLAG_BACKWARD);
    // avcodec_flush_buffers(ctx) — 清空解码器内部缓冲区（DPB、packet queue）
    //   seek 后必须调用，否则解码器可能输出 seek 前残留的帧数据
    avcodec_flush_buffers(ctx);

    // av_packet_alloc() / av_frame_alloc() — 分配并零初始化 AVPacket / AVFrame，不分配数据缓冲区
    AVPacket *pkt  = av_packet_alloc();
    AVFrame  *frame = av_frame_alloc();
    bool found = false;

    // av_read_frame(ctx, pkt) — 从容器读取下一个数据包（压缩帧），填充 pkt
    //   成功返回 0，到达文件末尾或出错返回负数；调用者需用 av_packet_unref 释放数据
    while (av_read_frame(fmt, pkt) >= 0 && !found) {
        if (pkt->stream_index == videoStream) {
            // avcodec_send_packet(ctx, pkt) — 向解码器送入一个压缩包
            //   成功返回 0；EAGAIN 表示需先 receive_frame；其他负数为错误
            if (avcodec_send_packet(ctx, pkt) == 0) {
                // avcodec_receive_frame(ctx, frame) — 从解码器取出一个解码后的原始帧
                //   成功返回 0；EAGAIN 表示需继续送包；AVERROR_EOF 表示已刷完所有帧
                while (avcodec_receive_frame(ctx, frame) == 0 && !found) {
                    // sws_getContext(srcW, srcH, srcFmt, dstW, dstH, dstFmt, flags, ...)
                    //   创建像素格式/尺寸转换上下文。
                    //   此处将解码器原始格式（通常 YUV420P）转为 RGB24，尺寸不变，使用双线性插值。
                    //   返回 SwsContext 指针，不再使用时须调用 sws_freeContext 释放。
                    SwsContext *sws = sws_getContext(
                        ctx->width, ctx->height, ctx->pix_fmt,
                        ctx->width, ctx->height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                    // QImage::Format_RGB888 与 AV_PIX_FMT_RGB24 内存布局完全一致（R8G8B8，无 Alpha）
                    //   可直接将 img.bits() 作为 sws_scale 输出缓冲，无需额外拷贝
                    QImage img(ctx->width, ctx->height, QImage::Format_RGB888);
                    uint8_t *dst[1]  = { img.bits() };
                    int dstStride[1] = { img.bytesPerLine() };
                    // sws_scale(sws, src[], srcStride[], srcSliceY, srcSliceH, dst[], dstStride[])
                    //   执行转换，srcSliceY=0 / srcSliceH=height 表示处理整帧
                    sws_scale(sws, frame->data, frame->linesize, 0, ctx->height, dst, dstStride);
                    // sws_freeContext(sws) — 释放 SwsContext（无对应的 sws_freeContext2，直接传指针）
                    sws_freeContext(sws);
                    emit resultReady(img);
                    found = true;
                }
            }
        }
        // av_packet_unref(pkt) — 释放 pkt 引用的数据缓冲区，重置字段；pkt 结构本身可复用
        av_packet_unref(pkt);
    }

    if (!found)
        emit errorOccurred(QString("在 %1s 处未找到帧").arg(seconds, 0, 'f', 0));

    // av_frame_free(&frame) / av_packet_free(&pkt) — 释放数据缓冲区并销毁结构体，指针置 nullptr
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
}
