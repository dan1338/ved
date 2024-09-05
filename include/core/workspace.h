#pragma once

#include <algorithm>

#include "timeline.h"

#include "ffmpeg/io.h"
#include "core/time.h"
#include "core/workspace_properties.h"
#include "core/render_session.h"

#include "logging.h"

namespace core
{
    class Workspace
    {
    public:
        Workspace(WorkspaceProperties props);

        const WorkspaceProperties &get_props()
        {
            return _props;
        }

        void set_props(const WorkspaceProperties &props)
        {
            _props = props;
            properties_changed_event.notify(_props);
        }

        core::timestamp get_cursor() const
        {
            return _cursor;
        }

        void set_cursor(core::timestamp position, bool update_flags = true)
        {
            LOG_DEBUG(_logger, "Set cursor, ts = {}", position / 1.0s);

            if (position < 0s)
            {
                _cursor = 0s;
            }
            else if (position > _timeline.get_duration())
            {
                _cursor = _timeline.get_duration();

                // If we hit the end while playing, stop
                if (_preview_active)
                    stop_preview();
            }
            else
            {
                _cursor = position;
            }

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

            if (_cursor > _timeline.get_duration())
                _cursor = _timeline.get_duration();
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

        Timeline::TrackID get_active_track_id() const
        {
            return _active_track_id;
        }

        void set_active_track(Timeline::TrackID id)
        {
            _active_track_id = id;
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

        // Insert clip into currently active track at cursor position
        void add_clip(core::MediaFile media_file);

        Event<WorkspaceProperties&> properties_changed_event;
        Event<RenderSession&> begin_render_event;
        Event<> stop_render_event;

    private:
        logging::Logger *_logger;

        WorkspaceProperties _props;

        Timeline _timeline;
        Timeline::TrackID _active_track_id{};

        bool _force_preview_refresh{false};
        bool _preview_active{false};

        core::timestamp _cursor{0s};

        std::unique_ptr<RenderSession> _render_session;
    };
}

