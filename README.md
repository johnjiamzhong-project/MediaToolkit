# MediaToolkit

基于 Qt 5.14.2 + FFmpeg 的桌面媒体处理工具，用于系统学习 FFmpeg 核心 API。三个功能模块独立开发，共用同一套框架。

## 文档

| 文档 | 说明 |
|------|------|
| [docs/development-plan.md](docs/development-plan.md) | 开发计划：阶段划分与完成标准 |
| [docs/superpowers/plans/2026-04-26-mediatoolkit-code-plan.md](docs/superpowers/plans/2026-04-26-mediatoolkit-plan.md) | 实现计划（含完整代码） |

## 环境要求

| 项目 | 版本/路径 |
|------|-----------|
| 编译器 | VS2017 (MSVC) |
| UI 框架 | Qt 5.14.2 |
| 媒体库 | FFmpeg（via vcpkg，`D:\vcpkg`） |
| 构建系统 | CMake 3.16+ |
| C++ 标准 | C++17 |

## 构建

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## 功能模块

### 功能 1 — 媒体信息查看器

- 用 `avformat_open_input` 打开文件
- 用 `av_dump_format` 打印流信息
- Qt GUI 显示：时长、分辨率、编解码器、比特率
- 核心学习：`AVFormatContext`、`AVStream`、`AVCodecParameters`

### 功能 2 — 视频截图工具

- 解封装 → 解码 → 取第 N 帧
- `swscale` 转换像素格式为 RGB
- 保存为 PNG（用 Qt 的 `QImage`）
- 核心学习：`AVPacket`、`AVFrame`、`sws_scale`

### 功能 3 — 音频提取转换

- 从视频提取音频流
- `swresample` 重采样
- 输出 MP3 / WAV
- 核心学习：`AVCodecContext`、编码循环、`SwrContext`
