#include "ffmpeg/frame_converter.h"
#include "logging.h"
#include <stdexcept>

static auto logger = logging::get_logger("FrameConverter");

namespace ffmpeg
{
    FrameConverter::FrameConverter(AVPixelFormat target_format):
        _target_format(target_format)
    {
    }

    AVFrame *FrameConverter::convert(AVFrame *in_frame, int target_width, int target_height)
    {
        if (target_width != _target_width || target_height != _target_height)
        {
            alloc_out_frame(target_width, target_height);
        }

        _sws_ctx = sws_getCachedContext(
            _sws_ctx,
            in_frame->width, in_frame->height, (AVPixelFormat)in_frame->format,
            _target_width, _target_height, _target_format,
            _sws_flags, nullptr, nullptr, nullptr
        );

        if (int err = sws_scale_frame(_sws_ctx, _out_frame, in_frame); err <= 0)
        {
            throw std::runtime_error("sws_scale " + std::to_string(err));
        }

        return av_frame_clone(_out_frame);
    }

    void FrameConverter::alloc_out_frame(int w, int h)
    {
        if (_out_frame != nullptr)
        {
            LOG_DEBUG(logger, "Freeing old frame, w = {}, h = {}", _out_frame->width, _out_frame->height);
            av_frame_free(&_out_frame);
        }

        _out_frame = av_frame_alloc();
        _out_frame->width = _target_width = w;
        _out_frame->height = _target_height = h;
        _out_frame->format = _target_format;
        LOG_DEBUG(logger, "Allocating new frame, w = {}, h = {}", w, h);

        if (av_frame_get_buffer(_out_frame, 0) != 0)
        {
            throw std::runtime_error("av_frame_get_buffer");
        }
    }
}

