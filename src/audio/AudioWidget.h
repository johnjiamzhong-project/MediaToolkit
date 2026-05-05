#pragma once
#include <QWidget>

class QPushButton;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QLabel;
class QThread;

class AudioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioWidget(QWidget *parent = nullptr);
    ~AudioWidget();

signals:
    void requestExtract(const QString &inputPath, const QString &outputPath, const QString &format);
    void requestCancel();

private slots:
    void onBrowseInput();
    void onBrowseOutput();
    void onStartCancel();
    void onProgress(int percent);
    void onFinished(const QString &outputPath);
    void onError(const QString &message);

private:
    void updateOutputPath();
    void setRunning(bool running);

    QLineEdit    *m_inputEdit;
    QPushButton  *m_inputBrowse;
    QComboBox    *m_formatCombo;
    QLineEdit    *m_outputEdit;
    QPushButton  *m_outputBrowse;
    QProgressBar *m_progress;
    QPushButton  *m_startBtn;
    QLabel       *m_statusLabel;
    QThread      *m_thread;
    bool          m_running = false;
};
