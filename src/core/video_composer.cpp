#include "core/video_composer.h"
#include "logging.h"

static auto logger = logging::get_logger("VideoComposer");

namespace core
{
    VideoComposer::VideoComposer(core::Timeline &timeline, WorkspaceProperties &props):
        _timeline(timeline),
        _props(props),
        _frame_dt(core::timestamp(1s).count() / props.frame_rate)
    {
        seek(0s);

        for (auto &track : _timeline.get_tracks())
        {
            for (auto &clip : track.clips)
            {
                add_clip(clip);
            }
        }

        _timeline.clip_added_event.add_callback([this](auto &clip){
            add_clip(clip);
        });

        _timeline.clip_moved_event.add_callback([this](auto &clip){
            if (_sources.find(clip.id) != _sources.end())
                _sources.erase(clip.id);

            add_clip(clip);
        });
    }

    std::string VideoComposer::get_name()
    {
        return {};
    }

    bool VideoComposer::seek(core::timestamp position)
    {
        // The timestamps used with SyncMediaSource have to be a multiple of frame_dt
        const auto dt_aligned_position = position - (position % _frame_dt);

        LOG_DEBUG(logger, "Seek, asked = {}s, actual = {}", position / 1.0s, dt_aligned_position / 1.0s);

        _composition = std::make_unique<Composition>();
        _composition->start_position = dt_aligned_position;
        _composition->last_position = dt_aligned_position;

        return true;
    }

    bool VideoComposer::seek(int64_t byte_offset)
    {
        return false;
    }

    AVFrame* VideoComposer::next_frame(AVMediaType frame_type)
    {
        auto &ts = _composition->last_position;

        AVFrame *out_frame = av_frame_alloc();
        out_frame->pts = ts.count();
        out_frame->duration = _frame_dt.count();

        static SwsContext *sws{nullptr};

        out_frame->width = _props.video_width;
        out_frame->height = _props.video_height;
        out_frame->format = AVPixelFormat::AV_PIX_FMT_RGB24;

        if (av_frame_get_buffer(out_frame, 0) != 0) {
            throw std::runtime_error("av_frame_get_buffer @ render");
        }

        // Zero out frame
        memset(out_frame->data[0], 0, out_frame->linesize[0] * out_frame->height);

        LOG_DEBUG(logger, "Next frame, ts = {}s", ts / 1.0s);
        LOG_TRACE_L3(logger, "Begin compose");

        for (auto &track : _timeline.get_tracks())
        {
            auto clip_idx = track.clip_at(ts);
            
            if (!clip_idx.has_value()) // No clip here
            {
                continue;
            }

            auto &clip = track.clips[*clip_idx];
            auto &source = _sources.at(clip.id);

            AVFrame *clip_frame = source.frame_at(ts - clip.position + clip.start_time);

            if (!clip_frame)
            {
                LOG_DEBUG(logger, "No frame found");
                continue;
            }

            sws = sws_getCachedContext(sws,
                clip_frame->width, clip_frame->height, (AVPixelFormat)clip_frame->format,
                out_frame->width, out_frame->height, (AVPixelFormat)out_frame->format,
                0, nullptr, nullptr, nullptr
            );

            sws_scale(sws, clip_frame->data, clip_frame->linesize, 0, clip_frame->height, out_frame->data, out_frame->linesize);
            av_frame_unref(clip_frame);
        }

        LOG_TRACE_L3(logger, "End compose");

        _composition->frames.push_back(out_frame); // TODO: remove this
        ts += _frame_dt;

        return out_frame;
    }

    bool VideoComposer::has_stream(AVMediaType frame_type)
    {
        return (frame_type == AVMEDIA_TYPE_AUDIO || frame_type == AVMEDIA_TYPE_VIDEO);
    }

    void VideoComposer::add_clip(Timeline::Clip &clip)
    {
        // We only support forward reading of frames
        // check if clip is reachable
        if (clip.end_position() < _composition->start_position)
            return;

        _sources.emplace(clip.id, clip.file.path);
    }
}

