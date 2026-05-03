#include "MediaInfoWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

MediaInfoWidget::MediaInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    qRegisterMetaType<MediaInfo>();

    m_pathEdit   = new QLineEdit(this);
    m_browseBtn  = new QPushButton("浏览", this);
    m_analyzeBtn = new QPushButton("分析", this);
    m_output     = new QTextEdit(this);
    m_output->setReadOnly(true);

    auto *row = new QHBoxLayout;
    row->addWidget(m_pathEdit);
    row->addWidget(m_browseBtn);
    row->addWidget(m_analyzeBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(row);
    layout->addWidget(m_output);

    auto *worker = new MediaInfoWorker;
    m_thread = new QThread(this);
    worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished,             worker, &QObject::deleteLater);
    connect(this,   &MediaInfoWidget::requestStart,  worker, &MediaInfoWorker::doWork);
    connect(worker, &MediaInfoWorker::resultReady,   this,   &MediaInfoWidget::onResult);
    connect(worker, &MediaInfoWorker::errorOccurred, this,   &MediaInfoWidget::onError);

    m_thread->start();

    connect(m_browseBtn,  &QPushButton::clicked, this, &MediaInfoWidget::onBrowse);
    connect(m_analyzeBtn, &QPushButton::clicked, this, [this]() {
        if (m_pathEdit->text().isEmpty()) return;
        m_analyzeBtn->setEnabled(false);
        m_output->clear();
        emit requestStart(m_pathEdit->text());
    });
}

MediaInfoWidget::~MediaInfoWidget()
{
    m_thread->quit();
    m_thread->wait();
}

void MediaInfoWidget::onBrowse()
{
    QString path = QFileDialog::getOpenFileName(this, "选择媒体文件", {},
        "媒体文件 (*.mp4 *.mkv *.avi *.mov *.mp3 *.wav *.flac);;所有文件 (*)");
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

void MediaInfoWidget::onResult(const MediaInfo &info)
{
    m_analyzeBtn->setEnabled(true);
    QString text;
    text += QString("文件：%1\n").arg(info.filename);
    text += QString("时长：%1 ms\n").arg(info.durationMs);
    text += QString("比特率：%1 bps\n").arg(info.bitrate);
    if (!info.videoCodec.isEmpty())
        text += QString("视频：%1  %2x%3\n").arg(info.videoCodec).arg(info.width).arg(info.height);
    if (!info.audioCodec.isEmpty())
        text += QString("音频：%1  %2 Hz  %3 ch\n").arg(info.audioCodec).arg(info.sampleRate).arg(info.channels);
    m_output->setPlainText(text);
}

void MediaInfoWidget::onError(const QString &message)
{
    m_analyzeBtn->setEnabled(true);
    QMessageBox::warning(this, "错误", message);
}
