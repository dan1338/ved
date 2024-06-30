#pragma once

#include <stdexcept>

#include "headers.h"

namespace ffmpeg
{
    struct MediaStream
    {
        AVStream *handle;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;

        MediaStream(AVStream *handle):
            handle(handle)
        {
            codec = avcodec_find_decoder(handle->codecpar->codec_id);
            codec_ctx = avcodec_alloc_context3(codec);

            if (avcodec_parameters_to_context(codec_ctx, handle->codecpar) < 0) {
                throw std::runtime_error("avcodec_parameters_to_context");
            }

            if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
                throw std::runtime_error("avcodec_open2");
            }
        }

        MediaStream(MediaStream &&rhs):
            handle(rhs.handle),
            codec(rhs.codec),
            codec_ctx(rhs.codec_ctx)
        {
        }

        MediaStream(const MediaStream &rhs) = delete;
        MediaStream& operator=(const MediaStream &rhs) = delete;

        ~MediaStream()
        {
            avcodec_free_context(&codec_ctx);
        }

        AVMediaType type() const
        {
            return handle->codecpar->codec_type;
        }
    };
}
