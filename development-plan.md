# MediaToolkit 开发计划

## 开发顺序

按以下四个阶段顺序推进，每个阶段完成后独立可编译运行。

| 阶段 | 内容 | 完成标准 |
|------|------|---------|
| Phase 1 | 框架骨架 | CMake 配置通过，程序启动显示三个空标签页 |
| Phase 2 | 功能 1 — 媒体信息查看器 | 可读取并显示媒体文件的时长、编解码器、分辨率 |
| Phase 3 | 功能 2 — 视频截图工具 | 可从视频解码指定帧并保存 PNG，界面显示预览 |
| Phase 4 | 功能 3 — 音频提取转换 | 可从视频提取音频并输出 MP3 / WAV，进度条实时更新 |

## 阶段说明

### Phase 1 — 框架骨架

目标：搭建可编译的空壳，确认工具链正常。

- `CMakeLists.txt`：配置 Qt5、FFmpeg（vcpkg）、C++17
- `src/main.cpp` + `MainWindow`：创建 `QTabWidget`，插入三个空 Tab
- 各模块桩文件：Widget 和 Worker 各含最小占位实现

构建命令：
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Phase 2 — 功能 1：媒体信息查看器

目标：学习 `AVFormatContext`、`AVStream`、`AVCodecParameters`。

核心流程：`avformat_open_input` → `avformat_find_stream_info` → 读取流参数 → 发射 `resultReady(MediaInfo)` 信号

### Phase 3 — 功能 2：视频截图工具

目标：学习 `AVPacket`、`AVFrame`、`sws_scale`。

核心流程：解封装 → 解码 → 取第 N 帧 → `sws_scale` 转 RGB24 → `QImage` 保存 PNG

### Phase 4 — 功能 3：音频提取转换

目标：学习 `AVCodecContext`、`SwrContext`、编码循环。

核心流程：解封装 → 找音频流 → 解码 → `swr_convert` 重采样 → 编码 → 写 MP3 / WAV

## 相关文档

- 架构设计：[docs/superpowers/specs/2026-04-26-mediatoolkit-design.md](superpowers/specs/2026-04-26-mediatoolkit-design.md)
- 实现计划（含完整代码）：[docs/superpowers/plans/2026-04-26-mediatoolkit-plan.md](superpowers/plans/2026-04-26-mediatoolkit-plan.md)
