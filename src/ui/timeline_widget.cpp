#include "ui/timeline_widget.h"

#include "ui/main_window.h"
#include "fmt/format.h"
#include "imgui.h"

#include "logging.h"

#include <filesystem>

auto logger = logging::get_logger("TimelineWidget");

namespace ui
{
    TimelineWidget::TimelineWidget(MainWindow &window, Properties &props):
        Widget(window),
        _workspace(window._workspace),
        _props(props)
    {
    }

    void TimelineWidget::show()
    {
        auto &timeline = _workspace.get_timeline();
        auto &active_track = timeline.get_track(_workspace.get_active_track_id());

        if (ImGui::Begin(_widget_name, 0, _win_flags))
        {
            if (_workspace.is_preview_active())
            {
                if (ImGui::Button("Pause"))
                {
                    _workspace.stop_preview();
                }
            }
            else
            {
                if (ImGui::Button("Play"))
                {
                    _workspace.start_preview();
                }
            }

            ImGui::SameLine();
            ImGui::Text("Preview FPS: %.1f", 1.0 / (float)(_window._frame_delta / 1.0s));

            ImGui::SameLine();
            ImGui::Text("Clips in active track: %zu", active_track.clips.size());

            if (ImGui::Button("Add Track"))
            {
                timeline.add_track();
            }

            ImGui::SameLine();

            if (ImGui::Button("Properties"))
            {
                _window._show_workspace_props = true;
            }

            ImGui::SetNextItemWidth(100.0);
            ImGui::SameLine();

            float input_cursor = _workspace.get_cursor() / 1.0s;
            if (ImGui::InputFloat("Cursor (seconds)", &input_cursor, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                _workspace.set_cursor(core::timestamp_from_double(input_cursor));
            }

            ImGui::SetNextItemWidth(100.0);
            ImGui::SameLine();

            float input_duration = timeline.get_duration() / 1.0s;
            if (ImGui::InputFloat("Duration (seconds)", &input_duration, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                timeline.set_duration(core::timestamp_from_double(input_duration));
            }

            show_tracks();
        }

        ImGui::End();
    }

    void TimelineWidget::draw_cursor()
    {
        const auto win_pos = ImGui::GetWindowPos();
        const auto win_size = ImGui::GetWindowSize();

        float cursor_x = _clip_window_x + timestamp_to_winpos(_workspace.get_cursor(), _track_window_w);

        if (cursor_x < _track_window_x || cursor_x > _track_window_x + _track_window_w)
            return;

        const auto cursor_color = ImGui::GetColorU32({0.8, 0.8, 0.8, 1.0});

        auto *fg_draw_list = ImGui::GetForegroundDrawList();
        fg_draw_list->AddLine({cursor_x, win_pos.y}, {cursor_x, win_pos.y + win_size.y}, cursor_color, 3.0);
    }

    void TimelineWidget::show_tracks()
    {
        auto &timeline = _workspace.get_timeline();

        // Emplace track index into here to remove track
        // Make the assumtion that only one track can be removed per frame
        std::optional<core::Timeline::TrackID> track_to_remove;

        if (ImGui::BeginChild("Tracks", {}, 0, 0))
        {
            ImGui::Columns(2, "Track columns", false);
            ImGui::SetColumnWidth(0, 100.0);

            // Draw headers
            timeline.foreach_track([&](auto &track){
                bool is_focused = (track.id == _workspace.get_active_track_id());
                std::string name{fmt::format("Track header {}", track.id)};

                if (is_focused) ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.6, 0.3, 0.3, 0.8});

                // Track header
                if (ImGui::BeginChild(name.c_str(), {0.0, _props.track_height}, _child_flags, 0))
                {
                    ImGui::Text("Track %d", (int)track.id);

                    ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});

                    if (ImGui::Button("X", {-1.0, -1.0}))
                    {
                        track_to_remove.emplace(track.id);
                    }

                    ImGui::PopStyleColor();

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        _workspace.set_active_track(track.id);
                    }
                }

                ImGui::EndChild();

                if (is_focused) ImGui::PopStyleColor();

                return true;
            });

            ImGui::NextColumn();

            // Draw clips
            if (ImGui::BeginChild("Track clips", {}, ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
            {
                timeline.foreach_track([&](auto &track){
                    std::string name{fmt::format("Track clips {}", track.id)};
                    bool is_focused = (track.id == _workspace.get_active_track_id());

                    const auto win_pos = ImGui::GetWindowPos();
                    const auto win_size = ImGui::GetWindowSize();

                    // Save dimensions for drawing the cursor
                    _track_window_x = win_pos.x;
                    _track_window_w = win_size.x;

                    const float total_width = timestamp_to_winpos(timeline.get_duration(), win_size.x);

                    // Track timeline
                    if (ImGui::BeginChild((name + "_clips").c_str(), {total_width, _props.track_height}, _child_flags, 0))
                    {
                        show_track_clips(track.id, is_focused, win_size.x);

                        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            _workspace.set_active_track(track.id);
                        }
                    }

                    ImGui::EndChild();

                    return true;
                });
            }

            ImGui::EndChild();

            // Draw cursor on top of tracks, requires use of ForegroundDrawList to overlay child windows
            draw_cursor();
        }

        ImGui::EndChild();

        // Remove track if asked to
        if (track_to_remove.has_value() && timeline.get_track_count() > 1)
        {
            timeline.rm_track(*track_to_remove);
        }
    }

    void TimelineWidget::MoveClip::update_impl(core::timestamp delta_t)
    {
        auto &clip = *init_state.clip;
        auto &track = timeline->get_track(clip.track_id);

        track.move_clip(clip, init_state.org_position + delta_t);
    }

    void TimelineWidget::ResizeClipLeft::update_impl(core::timestamp delta_t)
    {
        auto &clip = *init_state.clip;

        clip.position = init_state.org_position + delta_t;
        clip.duration = init_state.org_duration - delta_t;

        if (clip.duration > clip.max_duration())
        {
            clip.position -= delta_t;
            clip.duration = clip.max_duration();
        }

        timeline->clip_resized_event.notify(clip);
    }

    void TimelineWidget::ResizeClipRight::update_impl(core::timestamp delta_t)
    {
        auto &clip = *init_state.clip;
        clip.duration = init_state.org_duration + delta_t;

        if (clip.duration > clip.max_duration())
            clip.duration = clip.max_duration();

        timeline->clip_resized_event.notify(clip);
    }

    void TimelineWidget::show_track_clips(core::Timeline::TrackID track_id, bool is_focused, float parent_width)
    {
        auto &timeline = _workspace.get_timeline();
        auto &track = timeline.get_track(track_id);

        const auto win_pos = ImGui::GetWindowPos();
        const auto win_size = ImGui::GetWindowSize();

        LOG_DEBUG(logger, "parent_width {}, win_pos ({}, {}), win_size ({}, {})", parent_width, win_pos.x, win_pos.y, win_size.x, win_size.y);

        // Save dimensions for drawing the cursor
        _clip_window_x = win_pos.x;

        if (ImGui::IsWindowHovered())
        {
            const float track_x = (ImGui::GetMousePos().x - win_pos.x);
            const auto hover_clip_idx = track.clip_at(winpos_to_timestamp(track_x, parent_width));

            // Clip dragging behavior
            if (hover_clip_idx.has_value())
            {
                auto &clip = track.clips[*hover_clip_idx];
                const auto [px_start, px_end] = get_clip_pixel_bounds(clip, parent_width);

                const float dist_thresh = 10.0f;
                const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

                DraggingInitState init_state{&clip, clip.position, clip.duration};

                if (track_x - px_start < dist_thresh)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                    if (clicked)
                        setup_dragging_operation<ResizeClipLeft>(init_state);
                }
                else if (px_end - track_x < dist_thresh)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                    if (clicked)
                        setup_dragging_operation<ResizeClipRight>(init_state);
                }
                else if (clicked)
                {
                    setup_dragging_operation<MoveClip>(init_state);
                }
            }

            // Start handling dragging
            if (can_begin_dragging() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                LOG_INFO(logger, "Begin dragging clip");

                _dragging_operation->has_began = true;
            }

            // Update dragged clip / Finish dragging
            if (_dragging_operation != nullptr)
            {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    // Clip has beed dropped
                    LOG_INFO(logger, "Stop dragging clip");
                    _dragging_operation = nullptr;
                }
                else
                {
                    const auto delta_x = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
                    const core::timestamp delta_t = winpos_to_timestamp(delta_x, parent_width);

                    _dragging_operation->update(delta_t);
                }
            }
        }

        // Seek
        if (ImGui::IsWindowHovered())
        {
            auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

            // Released without dragging, set cursor location
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && delta.x == 0 && delta.y == 0)
            {
                auto [x, y] = ImGui::GetMousePos();
                _workspace.set_cursor(winpos_to_timestamp(x - win_pos.x, parent_width));
            }
        }

        // Hover highlight
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            //color = ImGui::GetColorU32({0.8, 0.4, 0.5, 1.0});
        }

        // Draw clips
        auto *draw_list = ImGui::GetWindowDrawList();
        const float x = win_pos.x;
        const float y = win_pos.y;
        const float w = win_size.x;
        const float h = _props.track_height;

        int bg_color = ImGui::GetColorU32({0.3, 0.3, 0.3, 1.0});
        int clip_color = ImGui::GetColorU32({0.8, 0.4, 0.5, 1.0});

        // Track background
        if (is_focused) {
            bg_color = ImGui::GetColorU32({0.45, 0.45, 0.45, 1.0});
        }

        draw_list->AddQuadFilled({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, bg_color);

        for (const auto &clip : track.clips)
        {
            const float clip_x = win_pos.x + timestamp_to_winpos(clip.position - _props.time_offset, parent_width);
            const float clip_w = timestamp_to_winpos(clip.duration, parent_width);

            // Clip rect
            draw_list->AddQuadFilled({clip_x, y}, {clip_x + clip_w, y}, {clip_x + clip_w, y + h}, {clip_x, y + h}, clip_color);

            // Clip filename
            std::filesystem::path path{clip.file.path};
            const auto [_, fontSize] = ImGui::CalcTextSize("");
            int padding = 8;
            draw_list->AddText({clip_x + padding, y + h - (fontSize * 2) - padding}, 0xffffffff, path.filename().c_str(), nullptr);
        }
    }
}

