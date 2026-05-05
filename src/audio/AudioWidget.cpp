#include "AudioWidget.h"
#include "AudioWorker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QMessageBox>

AudioWidget::AudioWidget(QWidget *parent)
    : QWidget(parent)
{
    m_inputEdit   = new QLineEdit(this);
    m_inputBrowse = new QPushButton("浏览", this);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("MP3", "mp3");
    m_formatCombo->addItem("WAV", "wav");

    m_outputEdit   = new QLineEdit(this);
    m_outputBrowse = new QPushButton("浏览", this);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);

    m_startBtn = new QPushButton("开始提取", this);

    m_statusLabel = new QLabel("请选择输入文件", this);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setStyleSheet("font-size: 9pt; color: gray;");

    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(new QLabel("输入:", this));
    inputRow->addWidget(m_inputEdit);
    inputRow->addWidget(m_inputBrowse);

    auto *fmtRow = new QHBoxLayout;
    fmtRow->addWidget(new QLabel("格式:", this));
    fmtRow->addWidget(m_formatCombo);
    fmtRow->addSpacing(16);
    fmtRow->addWidget(new QLabel("输出:", this));
    fmtRow->addWidget(m_outputEdit, 1);
    fmtRow->addWidget(m_outputBrowse);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addLayout(inputRow);
    layout->addLayout(fmtRow);
    layout->addWidget(m_progress);
    layout->addWidget(m_startBtn);
    layout->addWidget(m_statusLabel);
    layout->addStretch();

    auto *worker = new AudioWorker;
    m_thread = new QThread(this);
    worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished,           worker, &QObject::deleteLater);
    connect(this, &AudioWidget::requestExtract,     worker, &AudioWorker::doExtract);
    connect(this, &AudioWidget::requestCancel,      worker, &AudioWorker::cancel);
    connect(worker, &AudioWorker::progressUpdated,  this,   &AudioWidget::onProgress);
    connect(worker, &AudioWorker::finished,         this,   &AudioWidget::onFinished);
    connect(worker, &AudioWorker::errorOccurred,    this,   &AudioWidget::onError);
    m_thread->start();

    connect(m_inputBrowse,  &QPushButton::clicked, this, &AudioWidget::onBrowseInput);
    connect(m_outputBrowse, &QPushButton::clicked, this, &AudioWidget::onBrowseOutput);
    connect(m_startBtn,     &QPushButton::clicked, this, &AudioWidget::onStartCancel);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateOutputPath(); });
    connect(m_inputEdit, &QLineEdit::textChanged,
            this, [this](const QString &) { updateOutputPath(); });

    QSettings settings("MediaToolkit", "AudioWidget");
    QString lastPath = settings.value("lastInputPath").toString();
    if (!lastPath.isEmpty()) {
        m_inputEdit->setText(lastPath);
        updateOutputPath();
    }
}

AudioWidget::~AudioWidget()
{
    m_thread->quit();
    m_thread->wait();
}

void AudioWidget::updateOutputPath()
{
    QString input = m_inputEdit->text();
    if (input.isEmpty()) { m_outputEdit->clear(); return; }
    QFileInfo fi(input);
    QString ext = m_formatCombo->currentData().toString();
    m_outputEdit->setText(fi.absolutePath() + "/" + fi.baseName() + "." + ext);
}

void AudioWidget::setRunning(bool running)
{
    m_running = running;
    m_inputBrowse->setEnabled(!running);
    m_outputBrowse->setEnabled(!running);
    m_formatCombo->setEnabled(!running);
    m_inputEdit->setEnabled(!running);
    m_outputEdit->setEnabled(!running);
    m_startBtn->setText(running ? "取消" : "开始提取");
    m_startBtn->setEnabled(true);
}

void AudioWidget::onBrowseInput()
{
    QString path = QFileDialog::getOpenFileName(this, "选择视频/音频文件", {},
        "媒体文件 (*.mp4 *.mkv *.avi *.mov *.mp3 *.aac *.flac *.ogg *.wav);;所有文件 (*)");
    if (!path.isEmpty()) {
        m_inputEdit->setText(path);
        QSettings("MediaToolkit", "AudioWidget").setValue("lastInputPath", path);
        updateOutputPath();
    }
}

void AudioWidget::onBrowseOutput()
{
    QString ext = m_formatCombo->currentData().toString();
    QString filter = ext.toUpper() + " 文件 (*." + ext + ")";
    QString path = QFileDialog::getSaveFileName(this, "选择输出文件", m_outputEdit->text(), filter);
    if (!path.isEmpty()) m_outputEdit->setText(path);
}

void AudioWidget::onStartCancel()
{
    if (m_running) {
        emit requestCancel();
        m_startBtn->setEnabled(false);
        m_startBtn->setText("正在取消...");
        return;
    }

    QString input  = m_inputEdit->text().trimmed();
    QString output = m_outputEdit->text().trimmed();
    if (input.isEmpty() || output.isEmpty()) {
        QMessageBox::warning(this, "错误", "请指定输入和输出文件路径");
        return;
    }

    setRunning(true);
    m_progress->setValue(0);
    m_statusLabel->setStyleSheet("font-size: 9pt; color: gray;");
    m_statusLabel->setText("正在提取...");

    QString fmt = m_formatCombo->currentData().toString();
    emit requestExtract(input, output, fmt);
}

void AudioWidget::onProgress(int percent)
{
    m_progress->setValue(percent);
}

void AudioWidget::onFinished(const QString &outputPath)
{
    setRunning(false);
    m_statusLabel->setStyleSheet("font-size: 9pt; color: green;");
    m_statusLabel->setText(QString("完成：%1").arg(outputPath));
}

void AudioWidget::onError(const QString &message)
{
    setRunning(false);
    m_statusLabel->setStyleSheet("font-size: 9pt; color: red;");
    m_statusLabel->setText(QString("错误：%1").arg(message));
}
