#include "ui/preview_widget.h"

#include "ui/main_window.h"
#include "ui/helpers.h"
#include "core/application.h"

#include "fmt/base.h"
#include "fmt/ranges.h"
#include "fmt/format.h"
#include "imgui.h"

#include "logging.h"
#include <thread>

template<>
struct fmt::formatter<ImDrawVert>: formatter<string_view>
{
    auto format(ImDrawVert v, format_context &ctx) const
    {
        return formatter<string_view>::format(fmt::format("({}, {})", v.pos.x, v.pos.y), ctx);
    }
};

static auto logger = logging::get_logger("PreviewWidget");

namespace ui
{
    PreviewWidget::PreviewWidget(MainWindow &window):
        Widget(window),
        _workspace(core::app->get_workspace())
    {
        _preview = std::make_unique<Preview>(_workspace.get_timeline(), _workspace.get_props());

        _cb_user.shader = create_shader(basic_vertex_src, image_fragment_src);
        glGenBuffers(1, &_cb_user.vbo);
        glGenBuffers(1, &_cb_user.ebo);
        glGenTextures(1, &_cb_user.texture);

        LOG_DEBUG(logger, "Shader loaded, id = {}", _cb_user.shader);

        _cb_user.uniform_image = glGetUniformLocation(_cb_user.shader, "image");
        _cb_user.uniform_screen_size = glGetUniformLocation(_cb_user.shader, "screen_size");
        _cb_user.uniform_image_size = glGetUniformLocation(_cb_user.shader, "image_size");

        LOG_DEBUG(logger, "Shader uniforms, image = {}", _cb_user.uniform_image);
        LOG_DEBUG(logger, "Shader uniforms, screen_size = {}", _cb_user.uniform_screen_size);
        LOG_DEBUG(logger, "Shader uniforms, image_size = {}", _cb_user.uniform_image_size);
        
        // Set the precise display time of current frame if it's shown for the first time
        // also calculate the end time and notify MainWindow of it
        _window._buffer_swapped_event.add_callback([this](core::timestamp display_time) {
            const bool set_display_time =
                _preview->last_frame && !_preview->last_frame_display_time.has_value();

            if (set_display_time)
            {
                _preview->last_frame_display_time.emplace(display_time);
            }
        });

        // Subscribe to all track changed events to send them over to the preview thread
        auto &timeline = _workspace.get_timeline();

        timeline.track_modified_event.add_callback([this, &timeline](auto track_id){
            const auto &track = timeline.get_track(track_id);

            Preview::TrackModified event{std::make_unique<core::Timeline::Track>(track)};
            _preview->in_track_events << Preview::TrackEvent{std::move(event)};
        });

        timeline.track_added_event.add_callback([this, &timeline](auto track_id){
            const auto &track = timeline.get_track(track_id);

            Preview::TrackAdded event{std::make_unique<core::Timeline::Track>(track)};
            _preview->in_track_events << Preview::TrackEvent{std::move(event)};
        });

        timeline.track_removed_event.add_callback([this](auto track_id){
            Preview::TrackRemoved event{track_id};
            _preview->in_track_events << Preview::TrackEvent{std::move(event)};
        });

        // Reload worker on properties change
        _workspace.properties_changed_event.add_callback([this](auto &props){
            _preview = std::make_unique<Preview>(_workspace.get_timeline(), props);
        });

        // Initialize preview worker
        _preview->in_seek << Preview::SeekRequest{++_preview->seek_id, 0s};
    }

    PreviewWidget::~PreviewWidget()
    {
        glDeleteBuffers(1, &_cb_user.vbo);
        glDeleteBuffers(1, &_cb_user.ebo);
        glDeleteTextures(1, &_cb_user.texture);
        _preview->in_seek.close();
    }

    void PreviewWidget::show()
    {
        // Check if we need to seek
        if (_workspace.should_refresh_preview())
        {
            const auto cursor = _workspace.get_cursor();

            LOG_DEBUG(logger, "Submitting seek request, seek_id = {}, cursor = {}", _preview->seek_id + 1, cursor / 1.0s);

            _preview->in_seek << Preview::SeekRequest{++_preview->seek_id, cursor};
            _preview->last_frame = nullptr;
        }

        bool should_pull_frame = !_preview->last_frame;
        core::timestamp frame_sync_time{0s};

        if (_workspace.is_preview_active())
        {
            // If current frame has ended, ask for a new one
            if (const auto *frame = _preview->last_frame)
            {
                if (!_preview->last_frame_display_time.has_value())
                {
                    LOG_WARNING(logger, "No display time, frame has not been displayed?");
                }
                else
                {
                    frame_sync_time =
                        _preview->last_frame_display_time.value() + core::timestamp{frame->duration};

                    const auto time_remaining = (frame_sync_time - now());

                    if (time_remaining <= 0ms)
                    {
                        should_pull_frame = true;
                        LOG_TRACE_L1(logger, "Frame expired, pts = {}", frame->pts);
                    }
                    else if (time_remaining <= 17ms) // ~60HZ
                    {
                        should_pull_frame = true;
                        LOG_TRACE_L1(logger, "Frame expiring soon, pts = {}", frame->pts);
                    }
                }
            }
        }

        // Try fetch a frame
        if (should_pull_frame && !_preview->out_frames.empty())
        {
            Preview::PreviewFrame frame;

            while (!_preview->out_frames.empty())
            {
                _preview->out_frames >> frame;

                // Discard the frame if it has old seek id. Newer frames OTW
                if (frame.first < _preview->seek_id)
                {
                    LOG_TRACE_L1(logger, "Frame fetched and discarded");
                    av_frame_unref(frame.second);
                }
                else
                {
                    if (_preview->last_frame)
                        av_frame_unref(_preview->last_frame);

                    LOG_TRACE_L1(logger, "Frame fetched and replaced as latest, pts = {}", frame.second->pts);
                    _preview->last_frame = frame.second;
                    _workspace.set_cursor(core::timestamp{_preview->last_frame->pts}, false);

                    if (!_preview->last_frame_display_time.has_value())
                    {
                        LOG_WARNING(logger, "No display time, frame has not been displayed?");
                    }
                    else
                    {
                        _window._frame_sync_time = frame_sync_time;

                        LOG_DEBUG(logger, "Expiring frame lifetime was ({} - {})",
                                _preview->last_frame_display_time.value() / 1.0s, frame_sync_time / 1.0s);
                    }

                    _preview->last_frame_display_time.reset();

                    break;
                }
            }
        }

        // Draw preview
        if (ImGui::Begin(_widget_name, 0, _win_flags))
        {
            auto *draw_list = ImGui::GetWindowDrawList();

            if (_preview->last_frame)
            {
                auto *frame = _preview->last_frame;
                _cb_user.img_size.x = frame->width;
                _cb_user.img_size.y = frame->height;

                LOG_TRACE_L2(logger, "Updating preview texture, pts = {}, img_size=({}, {})", frame->pts, frame->width, frame->height);

                glBindTexture(GL_TEXTURE_2D, _cb_user.texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
            }

            // Mouse interactions
            if (ImGui::IsWindowHovered())
            {
                // Start dragging if clicked
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    _dragging = true;
                }

                // Stop dragging if relased
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    _dragging = false;
                }

                if (const auto delta = ImGui::GetMouseDragDelta(); !(delta.x == 0.0 && delta.y == 0.0))
                {
                    // Start dragging if clicked on this window
                    if (_dragging)
                    {
                        ImGui::ResetMouseDragDelta();

                        auto &timeline = _workspace.get_timeline();
                        auto &active_track = timeline.get_track(_workspace.get_active_track_id());
                        const auto clip_id = active_track.clip_at(_workspace.get_cursor());

                        if (clip_id.has_value())
                        {
                            auto &clip = active_track.clips[*clip_id];

                            const auto win_size = ImGui::GetWindowSize();

                            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                            {
                                const auto s = (delta.y / win_size.y) * 0.5;
                                active_track.scale_clip(clip, s, s);
                            }
                            else
                            {
                                active_track.translate_clip(clip, delta.x / win_size.x, delta.y / win_size.y);
                            }
                        }
                    }
                }
            }
            else
            {
                // Stop dragging if mose escaped window
                _dragging = false;
            }

            draw_list->PushClipRectFullScreen();
            draw_list->AddCallback([](const ImDrawList *prev_list, const ImDrawCmd *cmd){
                auto *user = (CbUserData*)cmd->UserCallbackData;

                const float x = user->win_pos.x;
                const float y = user->win_pos.y;
                const float w = user->win_size.x;
                const float h = user->win_size.y;
                const float vw = user->vp_size.x;
                const float vh = user->vp_size.y;

                glUseProgram(user->shader);
                glUniform1i(user->uniform_image, 1);
                glUniform2f(user->uniform_screen_size, w, h);
                glUniform2f(user->uniform_image_size, user->img_size.x, user->img_size.y);

                float verts[] = {
                    (x + 0) / vw, (y + 0) / vh, 0.0, 0.0,
                    (x + w) / vw, (y + 0) / vh, 1.0, 0.0,
                    (x + w) / vw, (y + h) / vh, 1.0, 1.0,
                    (x + 0) / vw, (y + h) / vh, 0.0, 1.0
                };

                glBindBuffer(GL_ARRAY_BUFFER, user->vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STREAM_DRAW);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(0));
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2));

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, user->ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, prev_list->IdxBuffer.size() * sizeof(ImDrawIdx), prev_list->IdxBuffer.begin(), GL_STREAM_DRAW);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, user->texture);
                glGenerateMipmap(GL_TEXTURE_2D);

                // ImGui Opengl3 renderer always binds their white texture before draw calls
                // Switch it back to 0 so our texture is not replaced
                glActiveTexture(GL_TEXTURE0);
            }, &_cb_user);
            draw_list->AddRectFilled({}, {}, 0xffffffff);
            draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
            draw_list->PopClipRect();

            _cb_user.win_pos = ImGui::GetWindowPos();
            _cb_user.win_size = ImGui::GetWindowSize();
            _cb_user.vp_size = ImGui::GetMainViewport()->WorkSize;
        }

        ImGui::End();
    }
}
