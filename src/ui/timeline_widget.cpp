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
        auto &active_track = timeline.get_track(_workspace.get_active_track_idx());

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

            show_tracks();
        }

        ImGui::End();
    }

    void TimelineWidget::draw_cursor()
    {
        auto *fg_draw_list = ImGui::GetForegroundDrawList();

        const auto win_pos = ImGui::GetWindowPos();
        const auto win_size = ImGui::GetWindowSize();

        const auto cursor_color = ImGui::GetColorU32({0.8, 0.8, 0.8, 1.0});

        float cursor_x = _track_window_x + timestamp_to_winpos(_workspace.get_cursor(), {_track_window_w, 0.0});
        fg_draw_list->AddLine({cursor_x, win_pos.y}, {cursor_x, win_pos.y + win_size.y}, cursor_color, 3.0);
    }

    void TimelineWidget::show_tracks()
    {
        auto &timeline = _workspace.get_timeline();
        auto &tracks = timeline.get_tracks();

        // Emplace track index into here to remove track
        // Make the assumtion that only one track can be removed per frame
        std::optional<size_t> track_to_remove;

        if (ImGui::BeginChild("Tracks", {}, 0, 0))
        {
            for (size_t i = 0; i < tracks.size(); i++)
            {
                bool is_focused = (i == _workspace.get_active_track_idx());
                std::string name{fmt::format("Track {}", i)};

                if (is_focused) ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.6, 0.3, 0.3, 0.8});

                // Track header
                if (ImGui::BeginChild(name.c_str(), {80.0, _props.track_height}, _child_flags, 0))
                {
                    ImGui::Text("Track %d", (int)i);

                    ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});

                    if (ImGui::Button("X", {-1.0, -1.0}))
                    {
                        track_to_remove.emplace(i);
                    }

                    ImGui::PopStyleColor();

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        _workspace.set_active_track(i);
                    }
                }

                ImGui::EndChild();
                if (is_focused) ImGui::PopStyleColor();

                ImGui::SameLine();

                // Track timeline
                if (ImGui::BeginChild((name + "_clips").c_str(), {0.0, _props.track_height}, _child_flags, 0))
                {
                    show_track_clips(i);

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        _workspace.set_active_track(i);
                    }
                }

                ImGui::EndChild();

            }

            // Draw cursor on top of tracks, requires use of ForegroundDrawList to overlay child windows
            draw_cursor();
        }

        ImGui::EndChild();

        // Remove track if asked to
        if (track_to_remove.has_value() && tracks.size() > 1)
        {
            timeline.rm_track(*track_to_remove);
        }
    }

    void TimelineWidget::show_track_clips(size_t track_idx)
    {
        auto &timeline = _workspace.get_timeline();
        auto &track = timeline.get_track(track_idx);
        bool is_focused = (track_idx == _workspace.get_active_track_idx());

        const auto win_pos = ImGui::GetWindowPos();
        const auto win_size = ImGui::GetWindowSize();

        // Save dimensions for drawing the cursor
        _track_window_x = win_pos.x;
        _track_window_w = win_size.x;

        if (ImGui::IsWindowHovered())
        {
            // Begin dragging clip
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const float track_x = (ImGui::GetMousePos().x - win_pos.x);
                const core::timestamp position = _props.time_offset + winpos_to_timestamp({track_x, 0}, win_size);

                const auto clip_idx = track.clip_at(position);

                if (clip_idx.has_value())
                {
                    LOG_INFO(logger, "Start dragging clip @ {}", *clip_idx);
                    _dragging_state = BeginDragging{track_idx, *clip_idx, track.clips[*clip_idx].position};
                }
            }

            // Start handling dragging
            if (std::holds_alternative<BeginDragging>(_dragging_state) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                LOG_INFO(logger, "Continue dragging clip");
                _dragging_state = ContinueDragging{std::get<BeginDragging>(_dragging_state)};
            }

            // Update dragged clip / Finish dragging
            if (std::holds_alternative<ContinueDragging>(_dragging_state))
            {
                const auto &cont_dragging = std::get<ContinueDragging>(_dragging_state);

                const auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                const core::timestamp delta_t = winpos_to_timestamp(delta, win_size);

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    // Clip has beed dropped
                    LOG_INFO(logger, "Stop dragging clip");
                    _dragging_state = std::monostate{};
                }
                else
                {
                    // Still dragging
                    auto &clip = get_dragged_clip();
                    LOG_INFO(logger, "Update dragging clip {}", (cont_dragging.org_position + delta_t).count());
                    track.move_clip(clip, cont_dragging.org_position + delta_t);
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
                _workspace.set_cursor(winpos_to_timestamp({x - win_pos.x, 0}, win_size));
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
            const float clip_x = win_pos.x + timestamp_to_winpos(clip.position - _props.time_offset, win_size);
            const float clip_w = timestamp_to_winpos(clip.duration, win_size);

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

