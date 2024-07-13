#pragma once

#include "imgui/imgui.h"

#include "core/workspace.h"
#include "core/timeline.h"
#include "core/time.h"

#include "ui/widget_ids.h"

namespace ui
{
    class TimelineWidget
    {
    public:
        struct Properties
        {
            float track_height;
            core::timestamp time_offset;
            core::milliseconds visible_timespan;
        };

        TimelineWidget(core::Workspace &workspace, Properties &props):
            _workspace(workspace),
            _props(props)
        {
        }

        void show();

    private:
        static constexpr auto *_widget_name = ui::widget_ids::timeline;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        static constexpr int _child_flags = ImGuiChildFlags_Border;

        core::Workspace &_workspace;
        Properties &_props;

        struct PlaybackState
        {
            bool active{false};

            double start_time;
            double speed{1.0};
        } _playback_state;

        struct DraggingInfo
        {
            bool active{false};

            int track_idx;
            int clip_idx;

            core::timestamp org_position;
        } _dragging_info;

        core::Timeline::Clip &get_dragged_clip()
        {
            auto &track = _workspace.get_tracks()[_dragging_info.track_idx];
            return track.clips[_dragging_info.clip_idx];
        }

        void show_tracks();
        void show_track_clips(size_t track_idx);
        void draw_cursor();

        core::timestamp winpos_to_timestamp(ImVec2 win_pos, ImVec2 win_size)
        {
            auto offset = _props.visible_timespan * (win_pos.x / win_size.x);

            return ((int64_t)offset.count()) * 1ms;
        }

        float timestamp_to_winpos(core::timestamp ts, ImVec2 win_size)
        {
            return (win_size.x * ts) / _props.visible_timespan;
        }
    };
}

