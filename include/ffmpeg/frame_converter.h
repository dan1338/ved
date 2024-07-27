#pragma once

#include "headers.h"

namespace ffmpeg
{
    class FrameConverter
    {
    public:
        FrameConverter(AVPixelFormat target_format);

        AVFrame *convert(AVFrame *in_frame, int target_width = 0, int target_height = 0);

    private:
        void alloc_out_frame(int w, int h);

        AVPixelFormat _target_format;
        int _target_width{0};
        int _target_height{0};
        AVFrame *_out_frame{nullptr};

        int _sws_flags{0};
        SwsContext *_sws_ctx{nullptr};
    };
}

