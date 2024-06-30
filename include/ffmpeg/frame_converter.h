#pragma once

#include "headers.h"

namespace ffmpeg
{
    class FrameConverter
    {
    public:
        FrameConverter(AVPixelFormat target_fmt):
            _sws_ctx(nullptr),
            _target_fmt(target_fmt),
            _out_frame(av_frame_alloc()),
            _sws_flags(0)
        {
            _out_frame->format = target_fmt;
        }

        ~FrameConverter()
        {
            sws_freeContext(_sws_ctx);

            if (_sws_ctx)
                av_frame_free(&_out_frame);
        }

        AVFrame* convert(AVFrame *in_frame)
        {
            _out_frame->width = in_frame->width;
            _out_frame->height = in_frame->height;

            if (_prev_width != _out_frame->width || _prev_height != _out_frame->height) {
                // FIXME: we're not freeing the old frame buffer (if already present)
                av_frame_get_buffer(_out_frame, 0);
                _prev_width = _out_frame->width;
                _prev_height = _out_frame->height;
            }

            _sws_ctx = sws_getCachedContext(
                _sws_ctx,
                in_frame->width, in_frame->height, (AVPixelFormat)in_frame->format,
                in_frame->width, in_frame->height, _target_fmt,
                _sws_flags, nullptr, nullptr, nullptr
            );

            sws_scale(_sws_ctx,
                in_frame->data, in_frame->linesize, 0, in_frame->height,
                _out_frame->data, _out_frame->linesize
            );

            return av_frame_clone(_out_frame);
        }

    private:
        SwsContext *_sws_ctx;
        AVPixelFormat _target_fmt;
        AVFrame *_out_frame;
        int _sws_flags;
        int _prev_width{0}, _prev_height{0};
    };
}

