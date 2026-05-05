#include "mainwindow.h"
#include "media_info/MediaInfoWidget.h"
#include "screenshot/ScreenshotWidget.h"
#include "audio/AudioWidget.h"

#include <QTabWidget>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabs(new QTabWidget(this))
{
    setWindowTitle("MediaToolkit");
    resize(800, 600);

    m_tabs->addTab(new MediaInfoWidget(this),  "媒体信息");
    m_tabs->addTab(new ScreenshotWidget(this), "视频截图");
    m_tabs->addTab(new AudioWidget(this),      "音频提取");

    setCentralWidget(m_tabs);
}
