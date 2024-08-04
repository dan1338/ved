#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "core/media_source.h"
#include "core/sync_media_source.h"
#include "core/time.h"
#include "core/timeline.h"
#include "core/workspace_properties.h"

#include "ffmpeg/frame_converter.h"
#include "ffmpeg/media_source.h"

namespace core
{
    class VideoComposer : public MediaSource
    {
    public:
        VideoComposer(core::Timeline &timeline, WorkspaceProperties props);

        void update_properties(WorkspaceProperties props);
        void update_track(core::Timeline::Track &track);
        void remove_track(core::Timeline::TrackID id);

        std::string get_name() override;

        bool seek(core::timestamp position) override;
        bool seek(int64_t byte_offset) override;

        AVFrame* next_frame(AVMediaType frame_type) override;
        bool has_stream(AVMediaType frame_type) override;

    private:
        void add_clip(Timeline::Clip &clip);
        void add_track(Timeline::Track &track);

        core::WorkspaceProperties _props;
        core::timestamp _frame_dt;

        std::map<Timeline::TrackID, Timeline::Track> _tracks;

        // Each clip has a SyncMediaReader which allows for fetching a frame
        // at a precise timestamp.
        //
        // The reader caches previously fetched frames and will return them
        // without decoding, if a frame at that timestamp has already been fetched
        // and hasn't yet been evicted.
        //
        // It's important that the timestamps are aligned to the workspace frame rate,
        // to ensure cache hits.
        std::unordered_map<Timeline::ClipID, SyncMediaSource> _sources;

        // Each track has it's own FrameConverter for the purpose of transforming
        // its current frame according to the ClipTransformation
        //
        // Having one converter per track strikes a balance between number of allocated
        // temporary frames and frequency of reallocation due to size change
        std::unordered_map<Timeline::TrackID, ffmpeg::FrameConverter> _frame_converters;

        struct Composition
        {
            core::timestamp start_position;
            core::timestamp last_position;
            std::vector<AVFrame*> frames;
        };

        std::unique_ptr<Composition> _composition;
    };
}

