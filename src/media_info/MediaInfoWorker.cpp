#include "MediaInfoWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

MediaInfoWorker::MediaInfoWorker(QObject *parent) : QObject(parent) {}

void MediaInfoWorker::doWork(const QString &filePath)
{
    AVFormatContext *fmt = nullptr;
    // avformat_open_input(ctx, url, fmt, options)
    //   分配并填充 AVFormatContext，探测容器格式，打开文件。
    //   成功返回 0，失败返回负数错误码，ctx 置 nullptr。
    int ret = avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        char buf[256];
        // av_strerror(errnum, buf, size) — 将负数错误码转为可读字符串
        av_strerror(ret, buf, sizeof(buf));
        emit errorOccurred(QString("无法打开文件: %1").arg(buf));
        return;
    }

    // avformat_find_stream_info(ctx, options)
    //   读取并解码少量数据包，填充各流的编解码参数（部分格式头部不含完整参数）。
    //   成功返回 >= 0，失败返回负数错误码。
    ret = avformat_find_stream_info(fmt, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        // avformat_close_input(&ctx) — 关闭文件，释放 AVFormatContext 及内部资源，ctx 置 nullptr
        avformat_close_input(&fmt);
        emit errorOccurred(QString("无法读取流信息: %1").arg(buf));
        return;
    }

    MediaInfo info;
    info.filename   = filePath;
    // fmt->duration — 容器总时长，单位 AV_TIME_BASE（微秒）；AV_NOPTS_VALUE 表示未知
    info.durationMs = (fmt->duration != AV_NOPTS_VALUE) ? fmt->duration / 1000 : 0;
    // fmt->bit_rate — 容器级总比特率（bps），非单独音/视频流的比特率
    info.bitrate    = fmt->bit_rate;

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        // AVCodecParameters — 存储流的编解码参数（codec_id, width, height, sample_rate 等）
        AVCodecParameters *par = fmt->streams[i]->codecpar;
        // avcodec_descriptor_get(codec_id) — 按 codec_id 查询静态描述符（含名称、类型等）
        //   返回指向全局只读 AVCodecDescriptor 的指针，未知 id 返回 nullptr
        const AVCodecDescriptor *desc = avcodec_descriptor_get(par->codec_id);
        QString codecName = desc ? desc->name : "unknown";

        if (par->codec_type == AVMEDIA_TYPE_VIDEO && info.videoCodec.isEmpty()) {
            info.videoCodec = codecName;
            info.width      = par->width;
            info.height     = par->height;
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && info.audioCodec.isEmpty()) {
            info.audioCodec = codecName;
            info.sampleRate = par->sample_rate;
            // par->ch_layout.nb_channels — FFmpeg 5.x 新 channel layout API，替代旧的 par->channels
            info.channels   = par->ch_layout.nb_channels;
        }
    }

    avformat_close_input(&fmt);
    emit resultReady(info);
}
