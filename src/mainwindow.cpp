#include "mainwindow.h"

#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

static QWidget *makePlaceholder(const QString &text)
{
    auto *w = new QWidget;
    auto *layout = new QVBoxLayout(w);
    auto *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return w;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabs(new QTabWidget(this))
{
    setWindowTitle("MediaToolkit");
    resize(800, 600);

    m_tabs->addTab(makePlaceholder("媒体信息查看器\n（待实现）"), "媒体信息");
    m_tabs->addTab(makePlaceholder("视频截图工具\n（待实现）"),   "视频截图");
    m_tabs->addTab(makePlaceholder("音频提取转换\n（待实现）"),   "音频提取");

    setCentralWidget(m_tabs);
}
