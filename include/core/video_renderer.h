#pragma once 

#include <unordered_map>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include "ffmpeg/video_reader.h"
#include "core/timeline.h"
#include "core/workspace.h"

namespace core
{
    class VideoRenderer
    {
    public:
        VideoRenderer(core::Timeline &timeline, core::Workspace::Properties &props);

        std::vector<AVFrame*> render(core::timestamp start_time, core::timestamp duration);
        AVFrame *get_frame_at(core::timestamp position, size_t prerender_frames = 1);
        void invalidate_cache();

    private:
        core::Timeline &_timeline;
        core::Workspace::Properties &_props;

        using ClipID = Timeline::Clip::ID;
        std::unordered_map<ClipID, std::unique_ptr<ffmpeg::VideoReader>> _clip_readers;

        void add_clip_reader(const Timeline::Clip &clip)
        {
            _clip_readers.emplace(clip.id, std::make_unique<ffmpeg::VideoReader>(clip.file));
        }

        std::vector<AVFrame*> _cached_frames;
        core::timestamp _cached_frames_start{0s};

        void render_generator(core::timestamp start_time, core::timestamp duration, std::function<void(AVFrame*)> frame_ready);
    };
}

