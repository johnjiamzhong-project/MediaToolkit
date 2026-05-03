#pragma once
#include <QWidget>
#include "MediaInfoWorker.h"

class QPushButton;
class QTextEdit;
class QLineEdit;
class QThread;

class MediaInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MediaInfoWidget(QWidget *parent = nullptr);
    ~MediaInfoWidget();

signals:
    void requestStart(const QString &filePath);

private slots:
    void onBrowse();
    void onResult(const MediaInfo &info);
    void onError(const QString &message);

private:
    QLineEdit   *m_pathEdit;
    QPushButton *m_browseBtn;
    QPushButton *m_analyzeBtn;
    QTextEdit   *m_output;
    QThread     *m_thread;
};
