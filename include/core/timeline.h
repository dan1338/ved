#pragma once

#include <cstdint>
#include <chrono>
#include <vector>
#include <set>
#include <optional>
#include <functional>

#include "core/media_file.h"
#include "core/time.h"
#include "core/event.h"
#include "core/workspace_properties.h"

namespace core
{
    struct ClipTransform
    {
        core::timestamp rel_position{0s};

        float translate_x{0.0};
        float translate_y{0.0};

        float scale_x{1.0};
        float scale_y{1.0};

        float rotation{0.0};

        bool operator<(const ClipTransform &rhs) const
        {
            return rel_position < rhs.rel_position;
        }
    };

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

            std::set<ClipTransform> transforms;

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

            void translate_clip(Clip &clip, float dx, float dy);
            void scale_clip(Clip &clip, float dx, float dy);
            void rotate_clip(Clip &clip, float dr);
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
        Event<Clip&> clip_transformed_event;
        Event<size_t> track_removed_event;

    private:
        WorkspaceProperties &_props;

        uint32_t _clip_id_counter{0};
        std::vector<Track> _tracks;
    };
}

