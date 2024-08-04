cmake_minimum_required(VERSION 3.10)
project(ved)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

include(cmake/FFMpeg.cmake)

# TODO: figure out a way to ensure linkage with static ffmpeg libs only
# this and the set(CMAKE_FIND_LIBRARY_SUFFIXES) seems to work for now
set(CMAKE_PREFIX_PATH vendor/ffmpeg/ffmpeg-install)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavutil
    libavcodec
    libavformat
    libswscale
)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GL REQUIRED gl)
pkg_check_modules(GLEW REQUIRED glew)

add_subdirectory(vendor/fmt)
add_subdirectory(vendor/quill)
add_subdirectory(vendor/msd_channel)
add_subdirectory(vendor/charls)

include_directories(
    ${GLFW3_INCLUDE_DIRS}
    ${GL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    vendor
    vendor/imgui
    include
)

link_directories(
    ${GLFW3_LIBRARY_DIRS}
    ${GL_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
)

add_executable(ved
    src/main.cpp
    src/logging.cpp
    src/ffmpeg/io.cpp
    src/ffmpeg/frame_converter.cpp
    src/ffmpeg/media_source.cpp
    src/core/workspace.cpp
    src/core/timeline.cpp
    src/core/video_composer.cpp
    src/core/sync_media_source.cpp
    src/ui/helpers.cpp
    src/ui/import_widget.cpp
    src/ui/timeline_widget.cpp
    src/ui/main_window.cpp
    src/ui/preview_widget.cpp
    src/ui/workspace_properties_widget.cpp
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

set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

target_link_libraries(ved
    imgui
    fmt
    quill
    msd_channel
    charls
    dl
    pthread
    PkgConfig::FFMPEG
    ${GLFW3_LIBRARIES}
    ${GL_LIBRARIES}
    ${GLEW_LIBRARIES}
)
