include(ProcessorCount)

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ffmpeg)

execute_process(
    COMMAND ./configure --prefix=${FFMPEG_DIR}/ffmpeg-install --enable-libx264 --enable-libvpx --enable-gpl
    WORKING_DIRECTORY ${FFMPEG_DIR}
)

ProcessorCount(NCPU)

execute_process(
    COMMAND make -j${NCPU}
    WORKING_DIRECTORY ${FFMPEG_DIR}
)

execute_process(
    COMMAND make install -j${NCPU}
    WORKING_DIRECTORY ${FFMPEG_DIR}
)

