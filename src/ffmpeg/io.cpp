#include "ffmpeg/io.h"

#include "ffmpeg/headers.h"
#include "fmt/base.h"

namespace ffmpeg
{
    static bool is_file_static_image(AVFormatContext *format_ctx)
    {
        if (format_ctx->nb_streams != 1)
            return false;

        const auto *stream = format_ctx->streams[0];

        if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
            return false;

        // Both JFIF and EXIF JPEGs are resolved as MJPEG video stream
        // The stream usually has duration of 1
        if (stream->codecpar->codec_id == AV_CODEC_ID_MJPEG)
            return stream->duration <= 1;

        return stream->duration <= 0;
    }

    namespace io
    {
        core::MediaFile open_file(const std::string &path)
        {
            core::MediaFile file{core::MediaFile::VIDEO, path};

            auto *format_ctx = avformat_alloc_context();

            avformat_open_input(&format_ctx, path.c_str(), nullptr, nullptr);
            avformat_find_stream_info(format_ctx, nullptr);

            for (size_t i = 0; i < format_ctx->nb_streams; i++)
            {
                const auto *stream = format_ctx->streams[i];

                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    file.width = stream->codecpar->width;
                    file.height = stream->codecpar->height;
                }
            }

            if (is_file_static_image(format_ctx))
            {
                file.type = core::MediaFile::STATIC_IMAGE;
                file.duration = 0s;
            }
            else
            {
                file.duration = core::custom_duration<1, AV_TIME_BASE>(format_ctx->duration);
            }

            avformat_close_input(&format_ctx);

            return file;
        }
    }
}
