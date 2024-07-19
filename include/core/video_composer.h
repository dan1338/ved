#pragma once

#include <memory>
#include <unordered_map>

#include "core/time.h"
#include "core/media_source.h"
#include "core/timeline.h"
#include "core/workspace.h"
#include "ffmpeg/media_source.h"

#include "logging.h"
#include "magic_enum.hpp"

namespace core
{
    class VideoComposer : public MediaSource
    {
    public:
        VideoComposer(core::Timeline &timeline, core::Workspace::Properties &props):
            _timeline(timeline),
            _props(props),
            _frame_dt(core::timestamp(1s).count() / props.frame_rate)
        {
            seek(0s);

            _timeline.clip_added_event.add_callback([this](auto &track, auto &clip){
                _composition->add_clip(clip);
            });
        }

        std::string get_name() override
        {
            return {};
        }

        bool seek(core::timestamp position) override
        {
            //logging::info("VideoComposer::seek to {:.9f}s", position / 1.0s);

            _composition = std::make_unique<Composition>();
            _composition->start_position = position;
            _composition->last_position = position;

            for (auto &track : _timeline.tracks)
            {
                for (auto &clip : track.clips)
                {
                    _composition->add_clip(clip);
                }
            }

            return true;
        }

        bool seek(int64_t byte_offset) override
        {
            return false;
        }

        AVFrame* next_frame(AVMediaType frame_type) override
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

            //logging::info("VideoComposer::next_frame ({}x{}) @ {:.9f} s", out_frame->width, out_frame->height, ts / 1.0s);

            for (auto &track : _timeline.tracks)
            {
                auto clip_idx = track.clip_at(ts);
                
                if (!clip_idx.has_value()) // No clip here
                {
                    continue;
                }

                auto &clip = track.clips[*clip_idx];
                auto &clip_source = *_composition->sources.at(clip.id);

                AVFrame *clip_frame{nullptr};
                core::timestamp clip_ts;

                // Get frame which aligns with current timestamp
                do
                {
                    if (clip_frame)
                        av_frame_unref(clip_frame);

                    clip_frame = clip_source.next_frame(AVMEDIA_TYPE_VIDEO);

                    if (!clip_frame) // Early EOF
                    {
                        break;
                    }

                    clip_ts = core::timestamp(clip_frame->pts) + clip.position;
                }
                while (ts > clip_ts);

                if (!clip_frame)
                {
                    //logging::info("No frame found");
                    continue;
                }

                //logging::info("Have {} @ {} pts", magic_enum::enum_name(clip_frame->pict_type), clip_frame->pts);

                sws = sws_getCachedContext(sws,
                    clip_frame->width, clip_frame->height, (AVPixelFormat)clip_frame->format,
                    out_frame->width, out_frame->height, (AVPixelFormat)out_frame->format,
                    0, nullptr, nullptr, nullptr
                );

                sws_scale(sws, clip_frame->data, clip_frame->linesize, 0, clip_frame->height, out_frame->data, out_frame->linesize);
                av_frame_unref(clip_frame);
            }

            _composition->frames.push_back(out_frame); // TODO: remove this
            ts += _frame_dt;

            return out_frame;
        }

    private:
        core::Timeline &_timeline;
        core::Workspace::Properties &_props;
        core::timestamp _frame_dt;

        using MediaSourcePtr = std::unique_ptr<MediaSource>;

        struct Composition
        {
            core::timestamp start_position;
            core::timestamp last_position;
            std::vector<AVFrame*> frames;
            std::unordered_map<Timeline::Clip::ID, MediaSourcePtr> sources;

            void add_clip(Timeline::Clip &clip)
            {
                // We only support forward reading of frames
                // check if clip is reachable
                if (clip.end_position() < start_position)
                    return;

                core::timestamp seek_position{0s};

                // If the cursor is over the clip, seek to that position in the clip
                if (last_position >= clip.position && last_position <= clip.end_position())
                    seek_position = (last_position - clip.position);

                auto source = ffmpeg::open_media_source(clip.file.path);
                source->seek(seek_position + clip.start_time);

                //logging::info("Open clip {} @ {:.9f}s", clip.file.path, (seek_position + clip.start_time) / 1.0s);

                sources[clip.id] = std::move(source);
            }
        };

        std::unique_ptr<Composition> _composition;
    };
}

