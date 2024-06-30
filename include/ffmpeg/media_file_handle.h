#pragma once

#include <stdexcept>
#include <vector>

#include "headers.h"
#include "core/media_file.h"
#include "media_stream.h"

namespace ffmpeg
{
    struct MediaFileHandle
    {
        const core::MediaFile &file;
        AVFormatContext *format_ctx;
        std::vector<MediaStream> streams;

        MediaFileHandle(const core::MediaFile &file):
            file(file),
            format_ctx(avformat_alloc_context())
        {
            if (avformat_open_input(&format_ctx, file.path.c_str(), nullptr, nullptr) != 0) {
                throw std::runtime_error("avformat_open_input");
            }

            streams.reserve(format_ctx->nb_streams);

            for (uint32_t i = 0; i < format_ctx->nb_streams; i++) {
                auto stream = format_ctx->streams[i];
                streams.emplace_back(stream);
            }
        }

        ~MediaFileHandle()
        {
            avformat_close_input(&format_ctx);
        }

        std::vector<size_t> streams_of_type(AVMediaType type) const
        {
            std::vector<size_t> ret;

            for (size_t i = 0; i < streams.size(); i++) {
                if (streams[i].type() == type)
                {
                    ret.push_back(i);
                }
            }

            return ret;
        }
    };
}

