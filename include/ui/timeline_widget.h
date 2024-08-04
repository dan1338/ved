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
        float _clip_window_x{0.0f};
        float _track_window_x{0.0f};
        float _track_window_w{0.0f};

        struct DraggingInitState
        {
            core::Timeline::Clip *clip;
            core::timestamp org_position;
            core::timestamp org_duration;
        };

        struct DraggingOperation
        {
            DraggingInitState init_state;

            core::Timeline *timeline;
            core::timestamp last_delta{};
            bool has_began{false};

            void update(core::timestamp delta_t)
            {
                if (last_delta != delta_t)
                {
                    update_impl(delta_t);
                    last_delta = delta_t;
                }
            }

            virtual void update_impl(core::timestamp delta_t) = 0;
        };

        struct MoveClip : public DraggingOperation { void update_impl(core::timestamp) override; };
        struct ResizeClipLeft : public DraggingOperation { void update_impl(core::timestamp) override; };
        struct ResizeClipRight : public DraggingOperation { void update_impl(core::timestamp) override; };

        std::unique_ptr<DraggingOperation> _dragging_operation{};

        template<typename TOperation>
        void setup_dragging_operation(DraggingInitState init_state)
        {
            _dragging_operation = std::make_unique<TOperation>();
            _dragging_operation->init_state = init_state;
            _dragging_operation->timeline = &_workspace.get_timeline();
        }

        bool can_begin_dragging() const
        {
            return _dragging_operation && !_dragging_operation->has_began;
        }

        core::timestamp winpos_to_timestamp(float pos_x, float win_w) const
        {
            auto offset = _props.visible_timespan * (pos_x / win_w);

            return ((int64_t)offset.count()) * 1ms;
        }

        float timestamp_to_winpos(core::timestamp ts, float win_w) const
        {
            return (win_w * ts) / _props.visible_timespan;
        }

        std::pair<float, float> get_clip_pixel_bounds(const core::Timeline::Clip &clip, float win_w) const
        {
            const auto px_start = timestamp_to_winpos(clip.position, win_w);
            const auto px_end = timestamp_to_winpos(clip.end_position(), win_w);

            return {px_start, px_end};
        }

        void show_tracks();
        void show_track_clips(core::Timeline::TrackID track_id, bool is_focused, float parent_width);
        void draw_cursor();
    };
}

