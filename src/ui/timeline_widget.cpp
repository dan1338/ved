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
            ImGui::Text("Clips in active track: %zu", _workspace.get_active_track().clips.size());

            if (ImGui::Button("Add Track"))
            {
                _workspace.add_track();
            }

            ImGui::SameLine();

            if (ImGui::Button("Remove Track"))
            {
                _workspace.remove_track(_workspace.get_active_track_idx());
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
        float cursor_x = win_pos.x + timestamp_to_winpos(_workspace.get_cursor(), win_size);

        fg_draw_list->AddLine({cursor_x, win_pos.y}, {cursor_x, win_pos.y + win_size.y}, cursor_color, 3.0);
    }

    void TimelineWidget::show_tracks()
    {
        auto &tracks = _workspace.get_tracks();

        if (ImGui::BeginChild("Tracks", {}, 0, 0))
        {
            // Draw cursor on top of tracks, requires use of ForegroundDrawList to overlay child windows
            draw_cursor();

            for (size_t i = 0; i < tracks.size(); i++)
            {
                std::string name{fmt::format("Track {}", i)};

                if (ImGui::BeginChild(name.c_str(), {0.0, _props.track_height}, _child_flags, 0))
                {
                    show_track_clips(i);

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        _workspace.set_active_track(i);
                    }
                }

                ImGui::EndChild();
            }
        }

        ImGui::EndChild();
    }

    void TimelineWidget::show_track_clips(size_t track_idx)
    {
        auto &track = _workspace.get_tracks()[track_idx];
        bool is_focused = (track_idx == _workspace.get_active_track_idx());

        const auto win_pos = ImGui::GetWindowPos();
        const auto win_size = ImGui::GetWindowSize();

        // Begin dragging clip
        if (!_dragging_info.active && ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            float track_x = (ImGui::GetMousePos().x - win_pos.x);
            core::timestamp position = _props.time_offset + winpos_to_timestamp({track_x, 0}, win_size);

            auto clip_idx = track.clip_at(position);

            if (clip_idx.has_value()) {
                _dragging_info.active = true;
                _dragging_info.track_idx = track_idx;
                _dragging_info.clip_idx = *clip_idx;
                _dragging_info.org_position = get_dragged_clip().position;
            }
        }

        // Update dragged clip / Finish dragging
        if (_dragging_info.active)
        {
            auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            core::timestamp delta_t = winpos_to_timestamp(delta, win_size);

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                // Clip has beed dropped
                _dragging_info.active = false;
            }
            else
            {
                // Still dragging
                auto &clip = get_dragged_clip();
                clip.position = _workspace.align_timestamp(_dragging_info.org_position + delta_t);

                auto &timeline = _workspace.get_timeline();
                timeline.clip_moved_event.notify(track, clip);
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

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            //color = ImGui::GetColorU32({0.8, 0.4, 0.5, 1.0});
        }

        // Draw clips
        auto *draw_list = ImGui::GetWindowDrawList();
        const float x = win_pos.x;
        const float y = win_pos.y;
        const float w = win_size.x;
        const float h = 100.0;

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

