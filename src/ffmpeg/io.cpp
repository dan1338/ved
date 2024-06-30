#include "ffmpeg/io.h"

#include "ffmpeg/headers.h"
#include "fmt/base.h"

namespace ffmpeg
{
    namespace io
    {
        core::MediaFile open_file(const std::string &path)
        {
            core::MediaFile file{core::MediaFile::VIDEO, path};

            auto *format_ctx = avformat_alloc_context();

            avformat_open_input(&format_ctx, path.c_str(), nullptr, nullptr);
            avformat_find_stream_info(format_ctx, nullptr);

            file.duration = core::custom_duration<1, AV_TIME_BASE>(format_ctx->duration);

            avformat_close_input(&format_ctx);

            return file;
        }
    }
}
