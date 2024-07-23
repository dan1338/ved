#pragma once

#include <cstdint>
#include <chrono>
#include <vector>
#include <optional>
#include <functional>

#include "core/media_file.h"
#include "core/time.h"
#include "core/event.h"
#include "core/workspace_properties.h"

namespace core
{
    class Timeline
    {
    public:
        struct Track;

        struct Clip
        {
            using ID = uint32_t;

            Track &track;
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
            void move_clip(Clip &clip, core::timestamp new_position);
        };

        Timeline(WorkspaceProperties &props);

        std::vector<Track> &get_tracks()
        {
            return _tracks;
        }

        Track &add_track()
        {
            return _tracks.emplace_back(Track{this});
        }

        void rm_track(size_t idx)
        {
            _tracks.erase(_tracks.begin() + idx);
            track_removed_event.notify(idx);
        }

        Track &get_track(size_t idx)
        {
            return _tracks[idx];
        }

        Event<Clip&> clip_added_event;
        Event<Clip&> clip_moved_event;
        Event<size_t> track_removed_event;

    private:
        WorkspaceProperties &_props;

        uint32_t _clip_id_counter{0};
        std::vector<Track> _tracks;
    };
}

