#include "MediaInfoWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

MediaInfoWorker::MediaInfoWorker(QObject *parent) : QObject(parent) {}

void MediaInfoWorker::doWork(const QString &filePath)
{
    AVFormatContext *fmt = nullptr;
    int ret = avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        emit errorOccurred(QString("无法打开文件: %1").arg(buf));
        return;
    }

    ret = avformat_find_stream_info(fmt, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        avformat_close_input(&fmt);
        emit errorOccurred(QString("无法读取流信息: %1").arg(buf));
        return;
    }

    MediaInfo info;
    info.filename   = filePath;
    info.durationMs = (fmt->duration != AV_NOPTS_VALUE) ? fmt->duration / 1000 : 0;
    info.bitrate    = fmt->bit_rate;

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        AVCodecParameters *par = fmt->streams[i]->codecpar;
        const AVCodecDescriptor *desc = avcodec_descriptor_get(par->codec_id);
        QString codecName = desc ? desc->name : "unknown";

        if (par->codec_type == AVMEDIA_TYPE_VIDEO && info.videoCodec.isEmpty()) {
            info.videoCodec = codecName;
            info.width      = par->width;
            info.height     = par->height;
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && info.audioCodec.isEmpty()) {
            info.audioCodec = codecName;
            info.sampleRate = par->sample_rate;
            info.channels   = par->ch_layout.nb_channels;
        }
    }

    avformat_close_input(&fmt);
    emit resultReady(info);
}
