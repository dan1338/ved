#pragma once

#include <algorithm>

#include "timeline.h"

#include "ffmpeg/io.h"
#include "core/time.h"
#include "core/workspace_properties.h"

#include "logging.h"

namespace core
{
    class Workspace
    {
    public:
        Workspace(WorkspaceProperties props);

        WorkspaceProperties &get_props()
        {
            return _props;
        }

        core::timestamp get_cursor() const
        {
            return _cursor;
        }

        void set_cursor(core::timestamp position, bool update_flags = true)
        {
            LOG_DEBUG(_logger, "Set cursor, ts = {}", position / 1.0s);

            _cursor = position;

            if (update_flags)
                _force_preview_refresh = true;
        }

        bool should_refresh_preview(bool clear_flag = true)
        {
            bool value = _force_preview_refresh;

            if (clear_flag)
                _force_preview_refresh = false;

            return value;
        }

        void increment_cursor()
        {
            _cursor += _props.frame_dt();
            _force_preview_refresh = true;
        }

        void decrement_cursor()
        {
            _cursor -= _props.frame_dt();
            _force_preview_refresh = true;

            if (_cursor < 0s)
                _cursor = 0s;
        }

        Timeline &get_timeline()
        {
            return _timeline;
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
            LOG_INFO(_logger, "Start preview");

            _preview_active = true;
        }

        void stop_preview()
        {
            LOG_INFO(_logger, "Stop preview");

            _preview_active = false;
        }

        bool is_preview_active() const
        {
            return _preview_active;
        }

    private:
        logging::Logger *_logger;

        WorkspaceProperties _props;
        Timeline _timeline;
        size_t _active_track_idx{0};
        bool _force_preview_refresh{false};
        bool _preview_active{false};

        core::timestamp _cursor{0s};
    };
}

