#pragma once

#include <cstdint>
#include <chrono>
#include <vector>
#include <optional>
#include <functional>

#include "media_file.h"
#include "time.h"
#include "event.h"

namespace core
{
    struct Timeline
    {
        struct Clip
        {
            using ID = uint32_t;

            ID id;

            core::timestamp position;

            core::timestamp start_time;
            core::timestamp duration;

            MediaFile file;

            core::timestamp end_position() const 
            {
                return position + duration;
            }
        };

        struct Track
        {
            Track(Timeline *timeline): 
                timeline(timeline)
            {
            }

            Timeline *timeline;
            std::vector<Clip> clips;

            std::pair<core::timestamp, core::timestamp> bounds() const;

            // TODO: replace with sorted container
            std::optional<size_t> clip_at(core::timestamp position);

            void add_clip(core::MediaFile file, core::timestamp position = 0s);
        };

        uint32_t clip_id_counter{0};
        std::vector<Track> tracks;

        Track& add_track();

        Event<Track&, Clip&> clip_added_event;
        Event<Track&, Clip&> clip_moved_event;
    };
}

