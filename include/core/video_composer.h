#pragma once

#include <memory>
#include <unordered_map>

#include "core/time.h"
#include "core/media_source.h"
#include "core/cached_media_source.h"
#include "core/timeline.h"
#include "core/workspace.h"
#include "ffmpeg/media_source.h"

#include "magic_enum.hpp"

namespace core
{
    class VideoComposer : public MediaSource
    {
    public:
        VideoComposer(core::Timeline &timeline, core::Workspace::Properties &props);

        std::string get_name() override;

        bool seek(core::timestamp position) override;
        bool seek(int64_t byte_offset) override;

        AVFrame* next_frame(AVMediaType frame_type) override;

        bool has_stream(AVMediaType frame_type) override
        {
            return (frame_type == AVMEDIA_TYPE_AUDIO || frame_type == AVMEDIA_TYPE_VIDEO);
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

            void add_clip(Timeline::Clip &clip);
        };

        std::unique_ptr<Composition> _composition;
    };
}

