#pragma once

#include <memory>
#include <unordered_map>

#include "core/time.h"
#include "core/media_source.h"
#include "core/timeline.h"
#include "core/workspace.h"
#include "core/sync_media_source.h"
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
        bool has_stream(AVMediaType frame_type) override;

    private:
        void add_clip(Timeline::Clip &clip);

        core::Timeline &_timeline;
        core::Workspace::Properties &_props;
        core::timestamp _frame_dt;

        std::unordered_map<Timeline::Clip::ID, SyncMediaSource> _sources;

        using MediaSourcePtr = std::unique_ptr<MediaSource>;

        struct Composition
        {
            core::timestamp start_position;
            core::timestamp last_position;
            std::vector<AVFrame*> frames;
        };

        std::unique_ptr<Composition> _composition;
    };
}

