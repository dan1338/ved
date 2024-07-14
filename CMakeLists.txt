cmake_minimum_required(VERSION 3.10)
project(ved)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

include(cmake/FFMpeg.cmake)

set(CMAKE_PREFIX_PATH vendor/ffmpeg/ffmpeg-install)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(SWSCALE REQUIRED libswscale)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GL REQUIRED gl)
pkg_check_modules(GLEW REQUIRED glew)

add_subdirectory(vendor/fmt)
add_subdirectory(vendor/quill)
add_subdirectory(vendor/msd_channel)

include_directories(
    ${AVUTIL_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIRS}
    ${GLFW3_INCLUDE_DIRS}
    ${GL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    vendor
    vendor/imgui
    include
)

link_directories(
    ${AVUTIL_LIBRARY_DIRS}
    ${AVCODEC_LIBRARY_DIRS}
    ${AVFORMAT_LIBRARY_DIRS}
    ${SWSCALE_LIBRARY_DIRS}
    ${GLFW3_LIBRARY_DIRS}
    ${GL_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
)

add_executable(ved
    src/main.cpp
    src/ffmpeg/io.cpp
    src/ffmpeg/media_source.cpp
    src/core/workspace.cpp
    src/core/timeline.cpp
    src/core/video_renderer.cpp
    src/ui/import_widget.cpp
    src/ui/timeline_widget.cpp
    src/ui/main_window.cpp
    src/ui/preview_widget.cpp
)

add_library(imgui OBJECT
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/misc/cpp/imgui_stdlib.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui.cpp
)

target_link_libraries(ved
    imgui
    fmt
    quill
    msd_channel
    dl
    pthread
    ${AVCODEC_STATIC_LIBRARIES}
    ${AVUTIL_STATIC_LIBRARIES}
    ${AVFORMAT_STATIC_LIBRARIES}
    ${SWSCALE_STATIC_LIBRARIES}
    ${GLFW3_LIBRARIES}
    ${GL_LIBRARIES}
    ${GLEW_LIBRARIES}
)

