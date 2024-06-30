#pragma once

#include <string>

#include "ffmpeg/headers.h"
#include "core/time.h"

namespace core
{
    class MediaSource
    {
    public:
        virtual ~MediaSource() = default;

        virtual std::string get_name() = 0;

        virtual bool seek(core::timestamp position) = 0;
        virtual bool seek(int64_t byte_offset) = 0;

        virtual AVFrame* next_frame(AVMediaType frame_type) = 0;
    };
};

