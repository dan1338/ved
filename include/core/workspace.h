#pragma once

#include <algorithm>

#include "timeline.h"

#include "ffmpeg/io.h"
#include "core/time.h"

#include "logging.h"

namespace core
{
    class Workspace
    {
    public:
        struct Properties
        {
            int video_width;
            int video_height;
            int frame_rate;
        };

        Workspace(Properties &props);

        Properties &get_props()
        {
            return _props;
        }

        core::timestamp get_cursor()
        {
            return _cursor;
        }

        void set_cursor(core::timestamp position)
        {
            _cursor = position;
            _cursor_dirty = true;
        }

        void clean_cursor()
        {
            _cursor_dirty = false;
        }

        bool is_cursor_dirty() const
        {
            return _cursor_dirty;
        }

        void increment_cursor()
        {
            _cursor += _frame_dt;
            _cursor_dirty = true;
        }

        void decrement_cursor()
        {
            _cursor -= _frame_dt;
            _cursor_dirty = true;

            if (_cursor < 0s)
                _cursor = 0s;
        }

        Timeline &get_timeline()
        {
            return _timeline;
        }

        std::vector<Timeline::Track> &get_tracks()
        {
            return _timeline.tracks;
        }

        Timeline::Track &get_active_track()
        {
            return _timeline.tracks[_active_track_idx];
        }

        size_t get_active_track_idx() const
        {
            return _active_track_idx;
        }

        void set_active_track(size_t idx)
        {
            _active_track_idx = idx;
        }

        void start_preview()
        {
            _preview_active = true;
        }

        void stop_preview()
        {
            _preview_active = false;
        }

        bool is_preview_active() const
        {
            return _preview_active;
        }

        Timeline::Track& add_track()
        {
            return _timeline.add_track();
        }

        void remove_track(uint32_t idx);

    private:
        Properties &_props;
        Timeline _timeline;
        size_t _active_track_idx{0};
        bool _preview_active{false};

        core::timestamp _cursor{0s};
        bool _cursor_dirty{false};

        core::timestamp _frame_dt;
    };
}

