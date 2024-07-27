#pragma once

#include "imgui/imgui.h"

#include "core/workspace.h"
#include "core/timeline.h"
#include "core/time.h"

#include "ui/widget_ids.h"
#include "ui/widget.h"

#include <variant>

namespace ui
{
    class MainWindow;

    class TimelineWidget : public Widget
    {
    public:
        struct Properties
        {
            float track_height;
            core::timestamp time_offset;
            core::milliseconds visible_timespan;
        };

        TimelineWidget(MainWindow &window, Properties &props);

        void show() override;

    private:
        static constexpr auto *_widget_name = ui::widget_ids::timeline;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        static constexpr int _child_flags = ImGuiChildFlags_Border;

        core::Workspace &_workspace;
        Properties &_props;

        // We save the last track windows x,w
        // here as a workaround for ImGui::FindWindow not exactly working :)
        // Used for drawing the cursor on top of tracks
        float _track_window_x{0.0f};
        float _track_window_w{0.0f};

        struct DraggingInfo
        {
            size_t track_idx;
            size_t clip_idx;
            core::timestamp org_position;
        };

        struct BeginDragging : public DraggingInfo {};
        struct ContinueDragging : public DraggingInfo {};

        using DraggingState = std::variant<std::monostate, BeginDragging, ContinueDragging>;
        DraggingState _dragging_state;

        core::Timeline::Clip &get_dragged_clip()
        {
            DraggingInfo *info{nullptr};

            if (std::holds_alternative<BeginDragging>(_dragging_state))
                info = &std::get<BeginDragging>(_dragging_state);
            if (std::holds_alternative<ContinueDragging>(_dragging_state))
                info = &std::get<ContinueDragging>(_dragging_state);

            auto &track = _workspace.get_timeline().get_track(info->track_idx);

            return track.clips[info->clip_idx];
        }

        core::timestamp winpos_to_timestamp(ImVec2 win_pos, ImVec2 win_size)
        {
            auto offset = _props.visible_timespan * (win_pos.x / win_size.x);

            return ((int64_t)offset.count()) * 1ms;
        }

        float timestamp_to_winpos(core::timestamp ts, ImVec2 win_size)
        {
            return (win_size.x * ts) / _props.visible_timespan;
        }

        void show_tracks();
        void show_track_clips(size_t track_idx);
        void draw_cursor();
    };
}

