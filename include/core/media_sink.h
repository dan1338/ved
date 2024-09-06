#pragma once

#include <string>

#include "ffmpeg/headers.h"
#include "core/time.h"

namespace core
{
    class MediaSink
    {
    public:
        virtual ~MediaSink() = default;

        virtual std::string get_name() = 0;
        virtual void write_frame(AVMediaType frame_type, AVFrame *frame) = 0;
    };
};

