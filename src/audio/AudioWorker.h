#pragma once
#include <QObject>
#include <QString>
#include <atomic>

class AudioWorker : public QObject
{
    Q_OBJECT
public:
    explicit AudioWorker(QObject *parent = nullptr);

public slots:
    void doExtract(const QString &inputPath, const QString &outputPath, const QString &format);
    void cancel();

signals:
    void progressUpdated(int percent);
    void finished(const QString &outputPath);
    void errorOccurred(const QString &message);

private:
    std::atomic<bool> m_cancel{false};
};
