# MediaToolkit 开发进度

## 总览

| 阶段 | 内容 | 状态 | 完成日期 |
|------|------|------|---------|
| Phase 1 | 框架骨架 | ✅ 完成 | 2026-04-30 |
| Phase 2 | 媒体信息查看器 | ✅ 完成 | 2026-05-03 |
| Phase 3 | 视频截图工具 | 🔲 未开始 | — |
| Phase 4 | 音频提取转换 | 🔲 未开始 | — |

---

## Phase 1 — 框架骨架 ✅

**完成内容**
- `CMakeLists.txt`：Qt 5.14.2 (msvc2017_64)，C++17，WIN32 可执行，windeployqt 自动部署
- `src/main.cpp`：`QApplication` 入口
- `src/mainwindow.h / .cpp`：`QMainWindow` + `QTabWidget`，三个占位 Tab

**已知环境**
- 编译器：VS2019，工具集 v142（Qt msvc2017_64 与 v142 ABI 兼容）
- Qt 路径：`G:\Qt\Qt5.14.2\5.14.2\msvc2017_64`
- FFmpeg：vcpkg 尚未安装，Phase 2 开始前需先集成

---

## Phase 2 — 媒体信息查看器 ✅

**目标**：读取并展示媒体文件的时长、分辨率、编解码器、比特率

**核心 FFmpeg API**：`avformat_open_input` → `avformat_find_stream_info` → `AVStream` / `AVCodecParameters`

**完成内容**
- [x] 集成 FFmpeg（`G:\ffmpeg`，手动安装，CMakeLists 直接链接 `.lib`，POST_BUILD 复制 DLL）
- [x] `MediaInfoWorker`：后台线程调用 FFmpeg，解析容器 / 视频流 / 音频流信息
- [x] `MediaInfoWidget`：Qt 界面，文件拖放 + 表格展示结果
- [x] 替换 Tab 1 占位符

---

## Phase 3 — 视频截图工具 🔲

**目标**：在指定时间点截取视频帧，预览并保存 PNG

**核心 FFmpeg API**：`AVPacket` → `AVFrame` → `sws_scale` → `QImage`

**待完成**
- [ ] `ScreenshotWorker`：解封装 → 解码 → 取帧 → 转 RGB24
- [ ] `ScreenshotWidget`：时间点输入 + 预览 + 保存按钮
- [ ] 替换 Tab 2 占位符

---

## Phase 4 — 音频提取转换 🔲

**目标**：从视频提取音轨并输出 MP3 / WAV，实时进度条

**核心 FFmpeg API**：`AVCodecContext` → `SwrContext` → 编码循环

**待完成**
- [ ] `AudioWorker`：解封装 → 找音频流 → 解码 → 重采样 → 编码写文件
- [ ] `AudioWidget`：格式选择 + 输出路径 + 进度条
- [ ] 替换 Tab 3 占位符
