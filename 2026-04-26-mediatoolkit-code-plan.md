# MediaToolkit 实现计划

> **对于自动化执行代理：** 必须子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 构建一个基于 Qt + FFmpeg 的桌面媒体工具包，包含三个独立功能模块：媒体信息查看器、视频截图工具和音频提取/转换。

**架构：** MainWindow 承载一个 QTabWidget，包含三组 Widget+Worker 对。每个 Widget 拥有其 Worker 线程，并通过 Qt 信号/槽进行独占通信。Worker 包含所有 FFmpeg 逻辑；Widget 包含所有 Qt UI 逻辑，不引入 FFmpeg 头文件。

**技术栈：** Qt 5.14.2（Widgets）、通过 vcpkg（`D:\vcpkg`）使用的 FFmpeg、CMake 3.16+、VS2017 MSVC、C++17

**目录规则：**
- 每个模块位于独立子目录；不跨目录引用
- `Widget` 文件：仅包含 Qt UI，不引入 FFmpeg 头文件
- `Worker` 文件：仅包含 FFmpeg 逻辑，不调用 Qt UI 方法

**线程模型（每个模块）：**r

```
主线程                         子线程
───────────────────────────────────────────────
XxxWidget                      XxxWorker
  │                                │
  │─── requestStart(params) ──────▶│
  │                                │  FFmpeg 操作
  │◀── progressChanged(percent) ───│
  │◀── resultReady(result) ────────│
  │◀── errorOccurred(message) ─────│
```

**线程约束：**
- Worker：不调用 `QWidget` 方法
- Widget：不直接调用 Worker 方法——仅通过信号触发
- Widget 在任务执行期间禁用输入控件以防止重复进入

**错误处理：**
- Worker：检查所有 FFmpeg 返回值；失败时调用 `av_strerror()`，发出 `errorOccurred(QString)` 信号，立即返回
- Widget：`onError` 显示 `QMessageBox::warning`，重新启用按钮
- 不使用 C++ 异常；错误不跨模块边界传播

---

## 阶段 1：框架骨架

### 任务 1：CMakeLists.txt

**Files:**
- Create: `CMakeLists.txt`

- [ ] **Step 1: 创建 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(MediaToolkit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(FFMPEG REQUIRED)

add_executable(MediaToolkit
    src/main.cpp
    src/mainwindow/MainWindow.cpp
    src/media_info/MediaInfoWidget.cpp
    src/media_info/MediaInfoWorker.cpp
    src/screenshot/ScreenshotWidget.cpp
    src/screenshot/ScreenshotWorker.cpp
    src/audio/AudioWidget.cpp
    src/audio/AudioWorker.cpp
)

target_include_directories(MediaToolkit PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(MediaToolkit Qt5::Widgets ${FFMPEG_LIBRARIES})
```

### 任务 2：main.cpp 和 MainWindow

**Files:**
- Create: `src/main.cpp`
- Create: `src/mainwindow/MainWindow.h`
- Create: `src/mainwindow/MainWindow.cpp`

- [ ] **Step 1: 创建 src/main.cpp**

```cpp
#include <QApplication>
#include "mainwindow/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
```

- [ ] **Step 2: 创建 src/mainwindow/MainWindow.h**

```cpp
#pragma once
#include <QMainWindow>

class MediaInfoWidget;
class ScreenshotWidget;
class AudioWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};
```

- [ ] **Step 3: 创建 src/mainwindow/MainWindow.cpp**

```cpp
#include "MainWindow.h"
#include <QTabWidget>
#include "../media_info/MediaInfoWidget.h"
#include "../screenshot/ScreenshotWidget.h"
#include "../audio/AudioWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("MediaToolkit");
    resize(800, 600);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(new MediaInfoWidget(this), "媒体信息");
    tabs->addTab(new ScreenshotWidget(this), "视频截图");
    tabs->addTab(new AudioWidget(this), "音频提取");
    setCentralWidget(tabs);
}
```

### 任务 3：各模块桩文件

**Files:**
- Create: `src/media_info/MediaInfoWidget.h`
- Create: `src/media_info/MediaInfoWidget.cpp`
- Create: `src/media_info/MediaInfoWorker.h`
- Create: `src/media_info/MediaInfoWorker.cpp`
- Create: `src/screenshot/ScreenshotWidget.h`
- Create: `src/screenshot/ScreenshotWidget.cpp`
- Create: `src/screenshot/ScreenshotWorker.h`
- Create: `src/screenshot/ScreenshotWorker.cpp`
- Create: `src/audio/AudioWidget.h`
- Create: `src/audio/AudioWidget.cpp`
- Create: `src/audio/AudioWorker.h`
- Create: `src/audio/AudioWorker.cpp`

- [ ] **Step 1: 创建 src/media_info/MediaInfoWidget.h**

```cpp
#pragma once
#include <QWidget>

class MediaInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MediaInfoWidget(QWidget *parent = nullptr);
};
```

- [ ] **Step 2: 创建 src/media_info/MediaInfoWidget.cpp**

```cpp
#include "MediaInfoWidget.h"
#include <QLabel>
#include <QVBoxLayout>

MediaInfoWidget::MediaInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("媒体信息 — 待实现", this));
}
```

- [ ] **Step 3: 创建 src/media_info/MediaInfoWorker.h**

```cpp
#pragma once
#include <QObject>

class MediaInfoWorker : public QObject
{
    Q_OBJECT
public:
    explicit MediaInfoWorker(QObject *parent = nullptr);
};
```

- [ ] **Step 4: 创建 src/media_info/MediaInfoWorker.cpp**

```cpp
#include "MediaInfoWorker.h"
```

- [ ] **Step 5: 创建 src/screenshot/ScreenshotWidget.h**

```cpp
#pragma once
#include <QWidget>

class ScreenshotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenshotWidget(QWidget *parent = nullptr);
};
```

- [ ] **Step 6: 创建 src/screenshot/ScreenshotWidget.cpp**

```cpp
#include "ScreenshotWidget.h"
#include <QLabel>
#include <QVBoxLayout>

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("视频截图 — 待实现", this));
}
```

- [ ] **Step 7: 创建 src/screenshot/ScreenshotWorker.h**

```cpp
#pragma once
#include <QObject>

class ScreenshotWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScreenshotWorker(QObject *parent = nullptr);
};
```

- [ ] **Step 8: 创建 src/screenshot/ScreenshotWorker.cpp**

```cpp
#include "ScreenshotWorker.h"
```

- [ ] **Step 9: 创建 src/audio/AudioWidget.h**

```cpp
#pragma once
#include <QWidget>

class AudioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioWidget(QWidget *parent = nullptr);
};
```

- [ ] **Step 10: 创建 src/audio/AudioWidget.cpp**

```cpp
#include "AudioWidget.h"
#include <QLabel>
#include <QVBoxLayout>

AudioWidget::AudioWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("音频提取 — 待实现", this));
}
```

- [ ] **Step 11: 创建 src/audio/AudioWorker.h**

```cpp
#pragma once
#include <QObject>

class AudioWorker : public QObject
{
    Q_OBJECT
public:
    explicit AudioWorker(QObject *parent = nullptr);
};
```

- [ ] **Step 12: 创建 src/audio/AudioWorker.cpp**

```cpp
#include "AudioWorker.h"
```

### 任务 4：首次构建验证

- [ ] **Step 1: CMake 配置**

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Expected: 配置成功，无错误。

- [ ] **Step 2: 编译**

```bash
cmake --build build
```

Expected: 生成 `MediaToolkit.exe`，无编译错误；运行后窗口显示三个空标签页：媒体信息、视频截图、音频提取。

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt src/
git commit -m "feat: add framework skeleton with three stub tabs"
```

---

## 阶段 2：功能 1 — 媒体信息查看器

**Overview:** 输入媒体文件 → `avformat_open_input` + `avformat_find_stream_info` → 读取流参数 → 显示时长/分辨率/编解码器/比特率。核心 API: `AVFormatContext`、`AVStream`、`AVCodecParameters`。Worker 信号: `resultReady(MediaInfo)`（纯数据结构体）

### 任务 5：MediaInfo 数据结构 + Worker

**Files:**
- Modify: `src/media_info/MediaInfoWorker.h`
- Modify: `src/media_info/MediaInfoWorker.cpp`

- [ ] **Step 1: 更新 src/media_info/MediaInfoWorker.h**

```cpp
#pragma once
#include <QObject>
#include <QString>

struct MediaInfo {
    QString filename;
    qint64  durationMs = 0;
    qint64  bitrate    = 0;
    int     width      = 0;
    int     height     = 0;
    QString videoCodec;
    QString audioCodec;
    int     sampleRate = 0;
    int     channels   = 0;
};
Q_DECLARE_METATYPE(MediaInfo)

class MediaInfoWorker : public QObject
{
    Q_OBJECT
public:
    explicit MediaInfoWorker(QObject *parent = nullptr);

public slots:
    void doWork(const QString &filePath);

signals:
    void resultReady(const MediaInfo &info);
    void errorOccurred(const QString &message);
};
```

- [ ] **Step 2: 更新 src/media_info/MediaInfoWorker.cpp**

```cpp
#include "MediaInfoWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

MediaInfoWorker::MediaInfoWorker(QObject *parent) : QObject(parent) {}

void MediaInfoWorker::doWork(const QString &filePath)
{
    AVFormatContext *fmt = nullptr;
    int ret = avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        emit errorOccurred(QString("无法打开文件: %1").arg(buf));
        return;
    }

    ret = avformat_find_stream_info(fmt, nullptr);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        avformat_close_input(&fmt);
        emit errorOccurred(QString("无法读取流信息: %1").arg(buf));
        return;
    }

    MediaInfo info;
    info.filename   = filePath;
    info.durationMs = (fmt->duration != AV_NOPTS_VALUE) ? fmt->duration / 1000 : 0;
    info.bitrate    = fmt->bit_rate;

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        AVCodecParameters *par = fmt->streams[i]->codecpar;
        const AVCodecDescriptor *desc = avcodec_descriptor_get(par->codec_id);
        QString codecName = desc ? desc->name : "unknown";

        if (par->codec_type == AVMEDIA_TYPE_VIDEO && info.videoCodec.isEmpty()) {
            info.videoCodec = codecName;
            info.width      = par->width;
            info.height     = par->height;
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && info.audioCodec.isEmpty()) {
            info.audioCodec = codecName;
            info.sampleRate = par->sample_rate;
            info.channels   = par->channels;
        }
    }

    avformat_close_input(&fmt);
    emit resultReady(info);
}
```

### 任务 6：MediaInfoWidget UI + 线程连接

**Files:**
- Modify: `src/media_info/MediaInfoWidget.h`
- Modify: `src/media_info/MediaInfoWidget.cpp`

- [ ] **Step 1: 更新 src/media_info/MediaInfoWidget.h**

```cpp
#pragma once
#include <QWidget>
#include "MediaInfoWorker.h"

class QPushButton;
class QTextEdit;
class QLineEdit;

class MediaInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MediaInfoWidget(QWidget *parent = nullptr);

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
};
```

- [ ] **Step 2: 更新 src/media_info/MediaInfoWidget.cpp**

```cpp
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
    auto *thread = new QThread(this);
    worker->moveToThread(thread);

    connect(thread, &QThread::finished,              worker, &QObject::deleteLater);
    connect(this,   &MediaInfoWidget::requestStart,  worker, &MediaInfoWorker::doWork);
    connect(worker, &MediaInfoWorker::resultReady,   this,   &MediaInfoWidget::onResult);
    connect(worker, &MediaInfoWorker::errorOccurred, this,   &MediaInfoWidget::onError);

    thread->start();

    connect(m_browseBtn,  &QPushButton::clicked, this, &MediaInfoWidget::onBrowse);
    connect(m_analyzeBtn, &QPushButton::clicked, this, [this]() {
        if (m_pathEdit->text().isEmpty()) return;
        m_analyzeBtn->setEnabled(false);
        m_output->clear();
        emit requestStart(m_pathEdit->text());
    });
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
```

### 任务 7：构建并验证功能 1

- [ ] **Step 1: 编译**

```bash
cmake --build build
```

Expected: 编译无错误。

- [ ] **Step 2: 手动测试**

运行 `MediaToolkit.exe`，切换到"媒体信息"标签，点击"浏览"选择一个 mp4 文件，点击"分析"，确认输出区域显示时长、编解码器、分辨率等信息。

- [ ] **Step 3: Commit**

```bash
git add src/media_info/
git commit -m "feat: implement media info viewer (Feature 1)"
```

---

## 阶段 3：功能 2 — 视频截图工具

**Overview:** 输入媒体文件 + 目标帧序号 N → 解封装→解码→取第 N 帧→`sws_scale` 转 RGB→`QImage` 保存 PNG → PNG 写入磁盘 + 界面预览缩略图。核心 API: `AVPacket`、`AVFrame`、`SwsContext`。Worker 信号: `resultReady(QImage)`

### 任务 8：ScreenshotWorker

**Files:**
- Modify: `src/screenshot/ScreenshotWorker.h`
- Modify: `src/screenshot/ScreenshotWorker.cpp`

- [ ] **Step 1: 更新 src/screenshot/ScreenshotWorker.h**

```cpp
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
    void doWork(const QString &filePath, int frameIndex);

signals:
    void resultReady(const QImage &image);
    void errorOccurred(const QString &message);
};
```

- [ ] **Step 2: 更新 src/screenshot/ScreenshotWorker.cpp**

```cpp
#include "ScreenshotWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

ScreenshotWorker::ScreenshotWorker(QObject *parent) : QObject(parent) {}

void ScreenshotWorker::doWork(const QString &filePath, int frameIndex)
{
    AVFormatContext *fmt = nullptr;
    if (avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开文件");
        return;
    }
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        emit errorOccurred("无法读取流信息");
        return;
    }

    int videoStream = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream < 0) {
        avformat_close_input(&fmt);
        emit errorOccurred("未找到视频流");
        return;
    }

    AVCodecParameters *par = fmt->streams[videoStream]->codecpar;
    const AVCodec *codec   = avcodec_find_decoder(par->codec_id);
    AVCodecContext *ctx    = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, par);
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        avformat_close_input(&fmt);
        emit errorOccurred("无法打开解码器");
        return;
    }

    AVPacket *pkt   = av_packet_alloc();
    AVFrame  *frame = av_frame_alloc();
    int decoded = 0;
    bool found  = false;

    while (av_read_frame(fmt, pkt) >= 0 && !found) {
        if (pkt->stream_index == videoStream) {
            if (avcodec_send_packet(ctx, pkt) == 0) {
                while (avcodec_receive_frame(ctx, frame) == 0) {
                    if (decoded == frameIndex) {
                        SwsContext *sws = sws_getContext(
                            ctx->width, ctx->height, ctx->pix_fmt,
                            ctx->width, ctx->height, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);

                        QImage img(ctx->width, ctx->height, QImage::Format_RGB888);
                        uint8_t *dst[1]    = { img.bits() };
                        int dstStride[1]   = { img.bytesPerLine() };
                        sws_scale(sws, frame->data, frame->linesize, 0, ctx->height, dst, dstStride);
                        sws_freeContext(sws);

                        emit resultReady(img);
                        found = true;
                        break;
                    }
                    ++decoded;
                }
            }
        }
        av_packet_unref(pkt);
    }

    if (!found)
        emit errorOccurred(QString("未找到第 %1 帧（共解码约 %2 帧）").arg(frameIndex).arg(decoded));

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
}
```

### Task 9: ScreenshotWidget

**Files:**
- Modify: `src/screenshot/ScreenshotWidget.h`
- Modify: `src/screenshot/ScreenshotWidget.cpp`

- [ ] **Step 1: 更新 src/screenshot/ScreenshotWidget.h**

```cpp
#pragma once
#include <QWidget>
#include <QImage>

class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;

class ScreenshotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenshotWidget(QWidget *parent = nullptr);

signals:
    void requestStart(const QString &filePath, int frameIndex);

private slots:
    void onBrowse();
    void onCapture();
    void onResult(const QImage &image);
    void onError(const QString &message);

private:
    QLineEdit   *m_pathEdit;
    QPushButton *m_browseBtn;
    QSpinBox    *m_frameSpinBox;
    QPushButton *m_captureBtn;
    QLabel      *m_preview;
};
```

- [ ] **Step 2: 更新 src/screenshot/ScreenshotWidget.cpp**

```cpp
#include "ScreenshotWidget.h"
#include "ScreenshotWorker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QPixmap>

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pathEdit     = new QLineEdit(this);
    m_browseBtn    = new QPushButton("浏览", this);
    m_frameSpinBox = new QSpinBox(this);
    m_frameSpinBox->setRange(0, 99999);
    m_frameSpinBox->setValue(0);
    m_captureBtn = new QPushButton("截图", this);
    m_preview    = new QLabel("预览", this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(320, 240);
    m_preview->setFrameShape(QFrame::Box);

    auto *row = new QHBoxLayout;
    row->addWidget(m_pathEdit);
    row->addWidget(m_browseBtn);
    row->addWidget(new QLabel("帧序号:", this));
    row->addWidget(m_frameSpinBox);
    row->addWidget(m_captureBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(row);
    layout->addWidget(m_preview);

    auto *worker = new ScreenshotWorker;
    auto *thread = new QThread(this);
    worker->moveToThread(thread);

    connect(thread, &QThread::finished,                worker, &QObject::deleteLater);
    connect(this,   &ScreenshotWidget::requestStart,   worker, &ScreenshotWorker::doWork);
    connect(worker, &ScreenshotWorker::resultReady,    this,   &ScreenshotWidget::onResult);
    connect(worker, &ScreenshotWorker::errorOccurred,  this,   &ScreenshotWidget::onError);

    thread->start();

    connect(m_browseBtn,  &QPushButton::clicked, this, &ScreenshotWidget::onBrowse);
    connect(m_captureBtn, &QPushButton::clicked, this, &ScreenshotWidget::onCapture);
}

void ScreenshotWidget::onBrowse()
{
    QString path = QFileDialog::getOpenFileName(this, "选择视频文件", {},
        "视频文件 (*.mp4 *.mkv *.avi *.mov);;所有文件 (*)");
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

void ScreenshotWidget::onCapture()
{
    if (m_pathEdit->text().isEmpty()) return;
    m_captureBtn->setEnabled(false);
    emit requestStart(m_pathEdit->text(), m_frameSpinBox->value());
}

void ScreenshotWidget::onResult(const QImage &image)
{
    m_captureBtn->setEnabled(true);

    QFileInfo fi(m_pathEdit->text());
    QString savePath = fi.absolutePath() + "/" + fi.baseName()
                       + QString("_frame%1.png").arg(m_frameSpinBox->value());
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
```

### Task 10: 构建并验证 Feature 2

- [ ] **Step 1: 编译**

```bash
cmake --build build
```

Expected: 编译无错误。

- [ ] **Step 2: 手动测试**

运行 `MediaToolkit.exe`，切换到"视频截图"标签，选择视频，帧序号填 0，点击"截图"，确认预览区显示第 0 帧画面，并在视频同目录生成 PNG 文件。

- [ ] **Step 3: Commit**

```bash
git add src/screenshot/
git commit -m "feat: implement video screenshot tool (Feature 2)"
```

---

## Phase 4: Feature 3 — 音频提取转换

**Overview:** 输入视频文件 + 输出格式（MP3/WAV）+ 输出路径 → 解封装→找音频流→解码→`swr_convert` 重采样→编码输出 → MP3 或 WAV 文件。核心 API: `AVCodecContext`、`SwrContext`、编码循环。Worker 信号: `progressChanged(int percent)`、`resultReady(QString outputPath)`

### Task 11: AudioWorker

**Files:**
- Modify: `src/audio/AudioWorker.h`
- Modify: `src/audio/AudioWorker.cpp`

- [ ] **Step 1: 更新 src/audio/AudioWorker.h**

```cpp
#pragma once
#include <QObject>
#include <QString>

class AudioWorker : public QObject
{
    Q_OBJECT
public:
    explicit AudioWorker(QObject *parent = nullptr);

public slots:
    void doWork(const QString &inputPath, const QString &outputPath, const QString &format);

signals:
    void progressChanged(int percent);
    void resultReady(const QString &outputPath);
    void errorOccurred(const QString &message);
};
```

- [ ] **Step 2: 更新 src/audio/AudioWorker.cpp**

```cpp
#include "AudioWorker.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

AudioWorker::AudioWorker(QObject *parent) : QObject(parent) {}

void AudioWorker::doWork(const QString &inputPath, const QString &outputPath, const QString &format)
{
    // Open input
    AVFormatContext *inFmt = nullptr;
    if (avformat_open_input(&inFmt, inputPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开输入文件");
        return;
    }
    avformat_find_stream_info(inFmt, nullptr);

    int audioStream = av_find_best_stream(inFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStream < 0) {
        avformat_close_input(&inFmt);
        emit errorOccurred("未找到音频流");
        return;
    }

    // Decoder
    AVCodecParameters *inPar  = inFmt->streams[audioStream]->codecpar;
    const AVCodec     *decoder = avcodec_find_decoder(inPar->codec_id);
    AVCodecContext    *decCtx  = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, inPar);
    avcodec_open2(decCtx, decoder, nullptr);

    // Output format context
    AVFormatContext *outFmt = nullptr;
    avformat_alloc_output_context2(&outFmt, nullptr, nullptr, outputPath.toUtf8().constData());
    if (!outFmt) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&inFmt);
        emit errorOccurred("无法创建输出格式");
        return;
    }

    // Encoder
    AVCodecID      encCodecId = (format == "mp3") ? AV_CODEC_ID_MP3 : AV_CODEC_ID_PCM_S16LE;
    const AVCodec *encoder    = avcodec_find_encoder(encCodecId);
    AVCodecContext *encCtx    = avcodec_alloc_context3(encoder);
    encCtx->sample_fmt     = encoder->sample_fmts[0];
    encCtx->sample_rate    = decCtx->sample_rate;
    encCtx->channel_layout = decCtx->channel_layout
                             ? decCtx->channel_layout
                             : av_get_default_channel_layout(decCtx->channels);
    encCtx->channels       = av_get_channel_layout_nb_channels(encCtx->channel_layout);
    encCtx->bit_rate       = 128000;
    if (outFmt->oformat->flags & AVFMT_GLOBALHEADER)
        encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    avcodec_open2(encCtx, encoder, nullptr);

    AVStream *outStream = avformat_new_stream(outFmt, encoder);
    avcodec_parameters_from_context(outStream->codecpar, encCtx);

    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
        avio_open(&outFmt->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE);
    avformat_write_header(outFmt, nullptr);

    // Resample context
    SwrContext *swr = swr_alloc_set_opts(nullptr,
        encCtx->channel_layout, encCtx->sample_fmt, encCtx->sample_rate,
        decCtx->channel_layout ? decCtx->channel_layout
                               : av_get_default_channel_layout(decCtx->channels),
        decCtx->sample_fmt, decCtx->sample_rate, 0, nullptr);
    swr_init(swr);

    qint64 totalDuration = inFmt->duration > 0 ? inFmt->duration : 1;
    AVPacket *inPkt    = av_packet_alloc();
    AVPacket *outPkt   = av_packet_alloc();
    AVFrame  *decFrame = av_frame_alloc();
    AVFrame  *encFrame = av_frame_alloc();

    encFrame->format         = encCtx->sample_fmt;
    encFrame->channel_layout = encCtx->channel_layout;
    encFrame->sample_rate    = encCtx->sample_rate;
    encFrame->nb_samples     = encCtx->frame_size > 0 ? encCtx->frame_size : 1152;
    av_frame_get_buffer(encFrame, 0);

    int64_t pts = 0;

    while (av_read_frame(inFmt, inPkt) >= 0) {
        if (inPkt->stream_index == audioStream) {
            if (avcodec_send_packet(decCtx, inPkt) == 0) {
                while (avcodec_receive_frame(decCtx, decFrame) == 0) {
                    swr_convert(swr,
                        encFrame->data, encFrame->nb_samples,
                        (const uint8_t **)decFrame->data, decFrame->nb_samples);

                    encFrame->pts = pts;
                    pts += encFrame->nb_samples;

                    if (avcodec_send_frame(encCtx, encFrame) == 0) {
                        while (avcodec_receive_packet(encCtx, outPkt) == 0) {
                            av_packet_rescale_ts(outPkt, encCtx->time_base, outStream->time_base);
                            outPkt->stream_index = 0;
                            av_interleaved_write_frame(outFmt, outPkt);
                            av_packet_unref(outPkt);
                        }
                    }

                    if (decFrame->pts != AV_NOPTS_VALUE) {
                        double ts  = decFrame->pts * av_q2d(inFmt->streams[audioStream]->time_base);
                        int    pct = static_cast<int>(ts * AV_TIME_BASE * 100 / totalDuration);
                        emit progressChanged(qBound(0, pct, 99));
                    }
                }
            }
        }
        av_packet_unref(inPkt);
    }

    // Flush encoder
    avcodec_send_frame(encCtx, nullptr);
    while (avcodec_receive_packet(encCtx, outPkt) == 0) {
        av_packet_rescale_ts(outPkt, encCtx->time_base, outStream->time_base);
        outPkt->stream_index = 0;
        av_interleaved_write_frame(outFmt, outPkt);
        av_packet_unref(outPkt);
    }

    av_write_trailer(outFmt);

    av_frame_free(&encFrame);
    av_frame_free(&decFrame);
    av_packet_free(&inPkt);
    av_packet_free(&outPkt);
    swr_free(&swr);
    avcodec_free_context(&encCtx);
    avcodec_free_context(&decCtx);
    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
        avio_close(outFmt->pb);
    avformat_free_context(outFmt);
    avformat_close_input(&inFmt);

    emit progressChanged(100);
    emit resultReady(outputPath);
}
```

### Task 12: AudioWidget

**Files:**
- Modify: `src/audio/AudioWidget.h`
- Modify: `src/audio/AudioWidget.cpp`

- [ ] **Step 1: 更新 src/audio/AudioWidget.h**

```cpp
#pragma once
#include <QWidget>

class QPushButton;
class QLineEdit;
class QComboBox;
class QProgressBar;

class AudioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioWidget(QWidget *parent = nullptr);

signals:
    void requestStart(const QString &inputPath, const QString &outputPath, const QString &format);

private slots:
    void onBrowseInput();
    void onBrowseOutput();
    void onConvert();
    void onProgress(int percent);
    void onResult(const QString &outputPath);
    void onError(const QString &message);

private:
    QLineEdit    *m_inputEdit;
    QPushButton  *m_inputBrowse;
    QLineEdit    *m_outputEdit;
    QPushButton  *m_outputBrowse;
    QComboBox    *m_formatCombo;
    QPushButton  *m_convertBtn;
    QProgressBar *m_progress;
};
```

- [ ] **Step 2: 更新 src/audio/AudioWidget.cpp**

```cpp
#include "AudioWidget.h"
#include "AudioWorker.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

AudioWidget::AudioWidget(QWidget *parent)
    : QWidget(parent)
{
    m_inputEdit    = new QLineEdit(this);
    m_inputBrowse  = new QPushButton("浏览", this);
    m_outputEdit   = new QLineEdit(this);
    m_outputBrowse = new QPushButton("浏览", this);
    m_formatCombo  = new QComboBox(this);
    m_formatCombo->addItems({"mp3", "wav"});
    m_convertBtn = new QPushButton("开始转换", this);
    m_progress   = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);

    auto *form = new QFormLayout;

    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_inputEdit);
    inputRow->addWidget(m_inputBrowse);
    form->addRow("输入文件:", inputRow);

    auto *outputRow = new QHBoxLayout;
    outputRow->addWidget(m_outputEdit);
    outputRow->addWidget(m_outputBrowse);
    form->addRow("输出文件:", outputRow);
    form->addRow("输出格式:", m_formatCombo);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_convertBtn);
    layout->addWidget(m_progress);

    auto *worker = new AudioWorker;
    auto *thread = new QThread(this);
    worker->moveToThread(thread);

    connect(thread, &QThread::finished,           worker, &QObject::deleteLater);
    connect(this,   &AudioWidget::requestStart,   worker, &AudioWorker::doWork);
    connect(worker, &AudioWorker::progressChanged, this,  &AudioWidget::onProgress);
    connect(worker, &AudioWorker::resultReady,     this,  &AudioWidget::onResult);
    connect(worker, &AudioWorker::errorOccurred,   this,  &AudioWidget::onError);

    thread->start();

    connect(m_inputBrowse,  &QPushButton::clicked, this, &AudioWidget::onBrowseInput);
    connect(m_outputBrowse, &QPushButton::clicked, this, &AudioWidget::onBrowseOutput);
    connect(m_convertBtn,   &QPushButton::clicked, this, &AudioWidget::onConvert);
}

void AudioWidget::onBrowseInput()
{
    QString path = QFileDialog::getOpenFileName(this, "选择视频文件", {},
        "视频文件 (*.mp4 *.mkv *.avi *.mov);;所有文件 (*)");
    if (path.isEmpty()) return;
    m_inputEdit->setText(path);

    QString fmt     = m_formatCombo->currentText();
    QString outPath = path.left(path.lastIndexOf('.')) + "." + fmt;
    m_outputEdit->setText(outPath);
}

void AudioWidget::onBrowseOutput()
{
    QString fmt  = m_formatCombo->currentText();
    QString path = QFileDialog::getSaveFileName(this, "保存音频文件", {},
        QString("%1 文件 (*.%1);;所有文件 (*)").arg(fmt.toUpper()).arg(fmt));
    if (!path.isEmpty())
        m_outputEdit->setText(path);
}

void AudioWidget::onConvert()
{
    if (m_inputEdit->text().isEmpty() || m_outputEdit->text().isEmpty()) return;
    m_convertBtn->setEnabled(false);
    m_progress->setValue(0);
    emit requestStart(m_inputEdit->text(), m_outputEdit->text(), m_formatCombo->currentText());
}

void AudioWidget::onProgress(int percent)
{
    m_progress->setValue(percent);
}

void AudioWidget::onResult(const QString &outputPath)
{
    m_convertBtn->setEnabled(true);
    m_progress->setValue(100);
    QMessageBox::information(this, "完成", QString("音频已保存到：\n%1").arg(outputPath));
}

void AudioWidget::onError(const QString &message)
{
    m_convertBtn->setEnabled(true);
    QMessageBox::warning(this, "错误", message);
}
```

### Task 13: 构建并验证 Feature 3

- [ ] **Step 1: 编译**

```bash
cmake --build build
```

Expected: 编译无错误。

- [ ] **Step 2: 手动测试**

运行 `MediaToolkit.exe`，切换到"音频提取"标签，选择含音频的视频文件，选择 mp3 格式，点击"开始转换"，确认进度条推进并弹出完成提示，输出的 mp3 文件可正常播放。

- [ ] **Step 3: Commit**

```bash
git add src/audio/
git commit -m "feat: implement audio extraction tool (Feature 3)"
```
