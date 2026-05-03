#pragma once
#include <QWidget>
#include <QImage>

class QPushButton;
class QLineEdit;
class QTimeEdit;
class QLabel;
class QThread;

class ScreenshotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenshotWidget(QWidget *parent = nullptr);
    ~ScreenshotWidget();

signals:
    void requestProbe(const QString &filePath);
    void requestStart(const QString &filePath, double seconds);

private slots:
    void onBrowse();
    void onCapture();
    void onVideoInfo(double durationSec, qint64 totalFrames, int width, int height, double fps);
    void onResult(const QImage &image);
    void onError(const QString &message);

private:
    QLineEdit   *m_pathEdit;
    QPushButton *m_browseBtn;
    QTimeEdit   *m_timeEdit;
    QPushButton *m_captureBtn;
    QLabel      *m_infoLabel;
    QLabel      *m_preview;
    QThread     *m_thread;
};
