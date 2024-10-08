#pragma once

#include <cstdint>
#include <chrono>
#include <vector>
#include <set>
#include <optional>
#include <functional>
#include <map>

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

        ClipTransform as_origin_transform() const
        {
            auto ret{*this};
            ret.rel_position = 0s;

            return ret;
        }
    };

    class Timeline
    {
    public:
        struct Track;
        using TrackID = uint32_t;
        using ClipID = uint32_t;

        struct Clip
        {
            ClipID id;
            TrackID track_id;

            core::timestamp position;

            core::timestamp start_time;
            core::timestamp duration;

            MediaFile file;

            std::set<ClipTransform> transforms;

            core::timestamp end_position() const 
            {
                return position + duration;
            }

            core::timestamp max_duration() const
            {
                if (file.type == MediaFile::STATIC_IMAGE)
                {
                    return core::timestamp{INT64_MAX};
                }
                else
                {
                    return file.duration - start_time;
                }
            }
        };

        struct Track
        {
            Track(TrackID id, Timeline *timeline): 
                id(id), timeline(timeline)
            {
            }

            TrackID id;
            Timeline *timeline;
            std::map<ClipID, Clip> clips;

            std::pair<core::timestamp, core::timestamp> bounds() const;

            // TODO: replace with sorted container
            std::optional<ClipID> clip_at(core::timestamp position);

            Clip &add_clip(core::MediaFile file, core::timestamp position = 0s, std::optional<ClipTransform> origin_transform = {});
            void rm_clip(ClipID id);
            void move_clip(Clip &clip, core::timestamp new_position);
            void split_clip(Clip &clip, core::timestamp split_position);

            void translate_clip(Clip &clip, float dx, float dy);
            void scale_clip(Clip &clip, float dx, float dy);
            void rotate_clip(Clip &clip, float dr);

            bool operator<(const Track &rhs) const
            {
                return id < rhs.id;
            }
        };

        Timeline(WorkspaceProperties &props);

        Track &add_track()
        {
            TrackID next_id = _track_id_counter++;

            _tracks.emplace(next_id, Track{next_id, this});
            track_added_event.notify(next_id);

            return _tracks.at(next_id);
        }

        void rm_track(TrackID id)
        {
            _tracks.erase(id);
            track_removed_event.notify(id);
        }

        Track &get_track(TrackID id)
        {
            return _tracks.at(id);
        }

        size_t get_track_count()
        {
            return _tracks.size();
        }

        void foreach_track(const std::function<bool(Track&)> &fn)
        {
            for (auto &[track_id, track] : _tracks)
            {
                if (!fn(track))
                    break;
            }
        }

        void swap_tracks(TrackID lhs_id, TrackID rhs_id)
        {
            if (_tracks.find(lhs_id) == _tracks.end() || _tracks.find(rhs_id) == _tracks.end())
                return;

            auto &lhs_track = _tracks.at(lhs_id);
            auto &rhs_track = _tracks.at(rhs_id);

            std::swap(lhs_track.clips, rhs_track.clips);

            const auto fix_parent_id = [](TrackID parent_id){
                return [parent_id](auto &it){ it.second.track_id = parent_id; };
            };

            std::for_each(lhs_track.clips.begin(), lhs_track.clips.end(), fix_parent_id(lhs_id));
            std::for_each(rhs_track.clips.begin(), rhs_track.clips.end(), fix_parent_id(rhs_id));

            track_modified_event.notify(lhs_id);
            track_modified_event.notify(rhs_id);
        }

        core::timestamp get_duration() const
        {
            return _duration;
        }

        void set_duration(core::timestamp ts)
        {
            _duration = ts;
            duration_changed_event.notify(ts);
        }

        Event<Clip&> clip_added_event;
        Event<Clip&> clip_removed_event;
        Event<Clip&> clip_moved_event;
        Event<Clip&> clip_resized_event;
        Event<Clip&> clip_transformed_event;
        Event<TrackID> track_added_event;
        Event<TrackID> track_removed_event;
        Event<TrackID> track_modified_event;
        Event<core::timestamp> duration_changed_event;

    private:
        WorkspaceProperties &_props;
        core::timestamp _duration{30s};

        uint32_t _clip_id_counter{0};
        uint32_t _track_id_counter{0};
        std::map<TrackID, Track> _tracks;
    };
}

