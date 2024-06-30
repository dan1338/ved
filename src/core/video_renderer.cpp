#include "core/video_renderer.h"

namespace core
{
    VideoRenderer::VideoRenderer(core::Timeline &timeline, core::Workspace::Properties &props):
        _timeline(timeline),
        _props(props)
    {
        for (const auto &track : _timeline.tracks)
        {
            for (const auto &clip : track.clips)
            {
                add_clip_reader(clip);
            }
        }

        _timeline.clip_added_event.add_callback([this](auto &track, auto &clip){
            add_clip_reader(clip);
        });
    }

    void VideoRenderer::render_generator(core::timestamp start_time, core::timestamp duration, std::function<void(AVFrame*)> frame_ready)
    {
        core::timestamp ts{start_time};
        const core::timestamp end_time{start_time + duration};
        const core::timestamp frame_dt{core::timestamp{1s} / _props.frame_rate};

        SwsContext *sws{nullptr};

        while (ts < end_time) {
            auto *out_frame = av_frame_alloc();
            out_frame->width = _props.video_width;
            out_frame->height = _props.video_height;
            out_frame->format = AVPixelFormat::AV_PIX_FMT_RGB24;

            if (av_frame_get_buffer(out_frame, 0) != 0) {
                throw std::runtime_error("av_frame_get_buffer @ render");
            }

            for (auto &track : _timeline.tracks) {
                auto clip_idx = track.clip_at(ts);

                if (!clip_idx.has_value()) // No clip at this time in this track
                    continue;

                auto &clip = track.clips[*clip_idx];

                const auto &reader = _clip_readers.at(clip.id);
                auto *clip_frame = reader->frame_at(clip.start_time + ts - clip.position);

                if (!clip_frame) // Early EOF
                    break;

                sws = sws_getCachedContext(sws,
                    clip_frame->width, clip_frame->height, (AVPixelFormat)clip_frame->format,
                    out_frame->width, out_frame->height, (AVPixelFormat)out_frame->format,
                    0, nullptr, nullptr, nullptr
                );

                sws_scale(sws, clip_frame->data, clip_frame->linesize, 0, clip_frame->height, out_frame->data, out_frame->linesize);
                av_frame_unref(clip_frame);
            }

            frame_ready(out_frame);

            ts += frame_dt;
        }

        sws_freeContext(sws);
    }

    std::vector<AVFrame*> VideoRenderer::render(core::timestamp start_time, core::timestamp duration)
    {
        std::vector<AVFrame*> frames;
        frames.reserve(30);

        render_generator(start_time, duration, [&frames](AVFrame *frame){
            frames.push_back(frame);
        });

        return frames;
    }

    AVFrame *VideoRenderer::get_frame_at(core::timestamp position, size_t prerender_frames)
    {
        const size_t num_cached = _cached_frames.size();
        const core::timestamp frame_dt{core::timestamp{1s} / _props.frame_rate};
        const core::timestamp cached_frames_end{_cached_frames_start + (frame_dt * num_cached)};

        if (num_cached == 0 || position < _cached_frames_start || position > cached_frames_end)
        {
            invalidate_cache();

            _cached_frames = render(position, frame_dt * prerender_frames);
            _cached_frames_start = position;

            fmt::println("Prerendered ({}) frames", _cached_frames.size());
        }

        for (size_t i = 0; i < _cached_frames.size(); i++) {
            core::timestamp frame_t{_cached_frames_start + (frame_dt * i)};

            if (position >= frame_t) {
                return _cached_frames[i];
            }
        }

        throw std::runtime_error("get_frame_at @ renderer");
    }

    void VideoRenderer::invalidate_cache()
    {
        for (auto *frame : _cached_frames)
            av_frame_free(&frame);

        _cached_frames.clear();
        _cached_frames_start = -1s;
    }
}

