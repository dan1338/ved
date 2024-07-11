#include "ui/preview_widget.h"

#include "fmt/base.h"
#include "fmt/ranges.h"
#include "fmt/format.h"
#include "imgui.h"

#include "logging.h"

template<>
struct fmt::formatter<ImDrawVert>: formatter<string_view>
{
    auto format(ImDrawVert v, format_context &ctx) const
    {
        return formatter<string_view>::format(fmt::format("({}, {})", v.pos.x, v.pos.y), ctx);
    }
};

namespace ui
{
    PreviewWidget::PreviewWidget(core::Workspace &workspace):
        _workspace(workspace),
        _preview(Preview{{workspace.get_timeline(), workspace.get_props()}})
    {
        _cb_user.shader = create_shader(basic_vertex_src, image_fragment_src);
        glGenBuffers(1, &_cb_user.vbo);
        glGenBuffers(1, &_cb_user.ebo);
        glGenTextures(1, &_cb_user.texture);

        _cb_user.uniform_image = glGetUniformLocation(_cb_user.shader, "image");
        _cb_user.uniform_screen_size = glGetUniformLocation(_cb_user.shader, "screen_size");
        _cb_user.uniform_image_size = glGetUniformLocation(_cb_user.shader, "image_size");
    }

    PreviewWidget::~PreviewWidget()
    {
        glDeleteBuffers(1, &_cb_user.vbo);
        glDeleteBuffers(1, &_cb_user.ebo);
        glDeleteTextures(1, &_cb_user.texture);
    }

    void PreviewWidget::show()
    {
        // Check if we need to seek
        if (_workspace.is_cursor_dirty())
        {
            _preview.in_seek << Preview::SeekRequest{++_preview.seek_id, _workspace.get_cursor()};
            _preview.last_frame = nullptr;
        }

        // If we have no frame, try fetch it
        if (!_preview.last_frame)
        {
            Preview::PreviewFrame frame;

            while (!_preview.out_frames.empty())
            {
                _preview.out_frames >> frame;

                // If out_frame seek id is older, discard the frame
                if (frame.first < _preview.seek_id)
                {
                    av_frame_unref(frame.second);
                }
                else
                {
                    _preview.last_frame = frame.second;
                    break;
                }
            }

            // Set cursor?
        }

        // Keep pulling frames if playback enabled
        if (_workspace.is_preview_active() && !_preview.out_frames.empty())
        {
            Preview::PreviewFrame frame;
            _preview.out_frames >> frame;
            _preview.last_frame = frame.second;
            _workspace.set_cursor(core::timestamp(_preview.last_frame->pts));
        }

        if (ImGui::Begin(_widget_name, 0, _win_flags))
        {
            auto *draw_list = ImGui::GetWindowDrawList();

            if (_preview.last_frame)
            {
                auto *frame = _preview.last_frame;
                _cb_user.img_size.x = frame->width;
                _cb_user.img_size.y = frame->height;

                glBindTexture(GL_TEXTURE_2D, _cb_user.texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
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

