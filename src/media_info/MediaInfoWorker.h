#pragma once
#include <QObject>
#include <QString>

struct MediaInfo {
    QString filename;
    qint64  durationMs = 0;
    qint64  bitrate    = 0;
    int     width      = 0;
    int     height     = 0;
    QString videoCodec;
    QString audioCodec;
    int     sampleRate = 0;
    int     channels   = 0;
};
Q_DECLARE_METATYPE(MediaInfo)

class MediaInfoWorker : public QObject
{
    Q_OBJECT
public:
    explicit MediaInfoWorker(QObject *parent = nullptr);

public slots:
    void doWork(const QString &filePath);

signals:
    void resultReady(const MediaInfo &info);
    void errorOccurred(const QString &message);
};
