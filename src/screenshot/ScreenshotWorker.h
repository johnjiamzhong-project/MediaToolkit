#pragma once
#include <QObject>
#include <QImage>
#include <QString>

class ScreenshotWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScreenshotWorker(QObject *parent = nullptr);

public slots:
    void probeVideo(const QString &filePath);
    void doWork(const QString &filePath, double seconds);

signals:
    void videoInfoReady(double durationSec, qint64 totalFrames, int width, int height, double fps);
    void resultReady(const QImage &image);
    void errorOccurred(const QString &message);
};
