#pragma once

#include <memory>
#include <unordered_map>

#include "core/time.h"
#include "core/media_source.h"
#include "core/timeline.h"
#include "core/workspace_properties.h"
#include "core/sync_media_source.h"

#include "ffmpeg/media_source.h"
#include "ffmpeg/frame_converter.h"

namespace core
{
    class VideoComposer : public MediaSource
    {
    public:
        VideoComposer(core::Timeline &timeline, WorkspaceProperties &props);

        std::string get_name() override;

        bool seek(core::timestamp position) override;
        bool seek(int64_t byte_offset) override;

        AVFrame* next_frame(AVMediaType frame_type) override;
        bool has_stream(AVMediaType frame_type) override;

    private:
        void add_clip(Timeline::Clip &clip);
        void add_track(Timeline::Track &track);

        core::Timeline &_timeline;
        core::WorkspaceProperties &_props;
        core::timestamp _frame_dt;

        // Each clip has a SyncMediaReader which allows for fetching a frame
        // at a precise timestamp.
        //
        // The reader caches previously fetched frames and will return them
        // without decoding, if a frame at that timestamp has already been fetched
        // and hasn't yet been evicted.
        //
        // It's important that the timestamps are aligned to the workspace frame rate,
        // to ensure cache hits.
        std::unordered_map<Timeline::Clip::ID, SyncMediaSource> _sources;

        // Each track has it's own FrameConverter for the purpose of transforming
        // its current frame according to the ClipTransformation
        //
        // Having one converter per track strikes a balance between number of allocated
        // temporary frames and frequency of reallocation due to size change
        std::unordered_map<Timeline::Track::ID, ffmpeg::FrameConverter> _frame_converters;

        struct Composition
        {
            core::timestamp start_position;
            core::timestamp last_position;
            std::vector<AVFrame*> frames;
        };

        std::unique_ptr<Composition> _composition;
    };
}

