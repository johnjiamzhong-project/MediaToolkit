cmake_minimum_required(VERSION 3.16)
project(MediaToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "G:/Qt/Qt5.14.2/5.14.2/msvc2017_64")

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(FFMPEG_ROOT "G:/ffmpeg")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")
set(FFMPEG_LIBRARIES
    "${FFMPEG_ROOT}/lib/avformat.lib"
    "${FFMPEG_ROOT}/lib/avcodec.lib"
    "${FFMPEG_ROOT}/lib/avutil.lib"
    "${FFMPEG_ROOT}/lib/swscale.lib"
    "${FFMPEG_ROOT}/lib/swresample.lib"
)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/media_info/MediaInfoWorker.cpp
    src/media_info/MediaInfoWidget.cpp
    src/screenshot/ScreenshotWorker.cpp
    src/screenshot/ScreenshotWidget.cpp
    src/audio/AudioWorker.cpp
    src/audio/AudioWidget.cpp
)

set(HEADERS
    src/mainwindow.h
    src/media_info/MediaInfoWorker.h
    src/media_info/MediaInfoWidget.h
    src/screenshot/ScreenshotWorker.h
    src/screenshot/ScreenshotWidget.h
    src/audio/AudioWorker.h
    src/audio/AudioWidget.h
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets ${FFMPEG_LIBRARIES})

# 部署 Qt DLL 到输出目录（仅 Windows）
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe"
                --no-translations
                $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Running windeployqt..."
    )
    # 复制 FFmpeg DLL
    foreach(_dll avformat-61 avcodec-61 avutil-59 swresample-5 swscale-8)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FFMPEG_ROOT}/bin/${_dll}.dll"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
endif()
