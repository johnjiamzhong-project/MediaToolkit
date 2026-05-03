#include "ScreenshotWidget.h"
#include "ScreenshotWorker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTimeEdit>
#include <QLabel>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QPixmap>
#include <QTime>
#include <QSettings>
#include <QSizePolicy>

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pathEdit  = new QLineEdit(this);
    m_browseBtn = new QPushButton("浏览", this);
    m_timeEdit  = new QTimeEdit(this);
    m_timeEdit->setDisplayFormat("HH:mm:ss.zzz");
    m_timeEdit->setTime(QTime(0, 0, 0, 0));
    m_captureBtn = new QPushButton("截图", this);

    m_infoLabel = new QLabel("请选择视频文件", this);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_infoLabel->setStyleSheet("font-size: 9pt; color: gray;");

    m_preview = new QLabel("预览", this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(320, 240);
    m_preview->setFrameShape(QFrame::Box);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *row = new QHBoxLayout;
    row->addWidget(m_pathEdit);
    row->addWidget(m_browseBtn);
    row->addWidget(new QLabel("时间:", this));
    row->addWidget(m_timeEdit);
    row->addWidget(m_captureBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(row);
    layout->addWidget(m_infoLabel);
    layout->addWidget(m_preview);
    layout->setStretchFactor(m_preview, 1);

    auto *worker = new ScreenshotWorker;
    m_thread = new QThread(this);
    worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished,                worker, &QObject::deleteLater);
    connect(this,     &ScreenshotWidget::requestProbe,   worker, &ScreenshotWorker::probeVideo);
    connect(this,     &ScreenshotWidget::requestStart,   worker, &ScreenshotWorker::doWork);
    connect(worker,   &ScreenshotWorker::videoInfoReady, this,   &ScreenshotWidget::onVideoInfo);
    connect(worker,   &ScreenshotWorker::resultReady,    this,   &ScreenshotWidget::onResult);
    connect(worker,   &ScreenshotWorker::errorOccurred,  this,   &ScreenshotWidget::onError);

    m_thread->start();

    connect(m_browseBtn,  &QPushButton::clicked, this, &ScreenshotWidget::onBrowse);
    connect(m_captureBtn, &QPushButton::clicked, this, &ScreenshotWidget::onCapture);

    QSettings settings("MediaToolkit", "ScreenshotWidget");
    QString lastPath = settings.value("lastFilePath").toString();
    if (!lastPath.isEmpty()) {
        m_pathEdit->setText(lastPath);
        m_infoLabel->setText("读取视频信息中...");
        emit requestProbe(lastPath);
    }
}

ScreenshotWidget::~ScreenshotWidget()
{
    m_thread->quit();
    m_thread->wait();
}

void ScreenshotWidget::onBrowse()
{
    QString path = QFileDialog::getOpenFileName(this, "选择视频文件", {},
        "视频文件 (*.mp4 *.mkv *.avi *.mov);;所有文件 (*)");
    if (!path.isEmpty()) {
        m_pathEdit->setText(path);
        m_infoLabel->setText("读取视频信息中...");
        QSettings("MediaToolkit", "ScreenshotWidget").setValue("lastFilePath", path);
        emit requestProbe(path);
    }
}

void ScreenshotWidget::onCapture()
{
    if (m_pathEdit->text().isEmpty()) return;
    m_captureBtn->setEnabled(false);
    QTime t = m_timeEdit->time();
    double seconds = t.hour() * 3600.0 + t.minute() * 60.0 + t.second() + t.msec() / 1000.0;
    emit requestStart(m_pathEdit->text(), seconds);
}

void ScreenshotWidget::onVideoInfo(double durationSec, qint64 totalFrames, int width, int height, double fps)
{
    if (durationSec <= 0) {
        m_infoLabel->setText("无法读取视频信息");
        return;
    }
    int h  = (int)(durationSec / 3600);
    int m  = (int)((durationSec - h * 3600) / 60);
    int s  = (int)(durationSec - h * 3600 - m * 60);
    int ms = (int)((durationSec - (int)durationSec) * 1000);
    m_timeEdit->setMaximumTime(QTime(h, m, s, ms));

    QString info = QString("时长: %1:%2:%3  |  总帧数: %4  |  分辨率: %5×%6  |  帧率: %7 fps")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(totalFrames)
        .arg(width).arg(height)
        .arg(fps, 0, 'f', 2);
    m_infoLabel->setText(info);
}

void ScreenshotWidget::onResult(const QImage &image)
{
    m_captureBtn->setEnabled(true);

    QFileInfo fi(m_pathEdit->text());
    QString timeStr = m_timeEdit->time().toString("HH_mm_ss_zzz");
    QString savePath = fi.absolutePath() + "/" + fi.baseName() + "_" + timeStr + ".png";
    image.save(savePath, "PNG");

    QPixmap pm = QPixmap::fromImage(image).scaled(
        m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_preview->setPixmap(pm);
    m_preview->setToolTip(savePath);
}

void ScreenshotWidget::onError(const QString &message)
{
    m_captureBtn->setEnabled(true);
    QMessageBox::warning(this, "错误", message);
}
