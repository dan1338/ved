#pragma once

#include "imgui/imgui.h"

#include "core/workspace.h"
#include "core/video_composer.h"
#include "ffmpeg/frame_converter.h"
#include "ui/widget_ids.h"
#include "ui/widget.h"

#include "msd/channel.hpp"

#include "shaders.h"
#include "logging.h"

#include <thread>
#include <atomic>
#include <memory>

namespace ui
{
    class MainWindow;

    class PreviewWidget : public Widget
    {
    public:
        PreviewWidget(MainWindow &window);
        ~PreviewWidget();

        void show() override;

    private:
        static constexpr auto *_widget_name = ui::widget_ids::preview;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::Workspace &_workspace;
        bool _dragging{false};
        
        struct Preview
        {
            core::VideoComposer composer;
            AVFrame *last_frame{nullptr};
            std::optional<core::timestamp> last_frame_display_time;
            uint64_t seek_id{0};

            using SeekRequest = std::pair<uint64_t, core::timestamp>;
            msd::channel<SeekRequest> in_seek;
            
            using PreviewFrame = std::pair<uint64_t, AVFrame*>;
            msd::channel<PreviewFrame> out_frames{1};

            std::thread thread{[this](){
                auto logger = logging::get_logger("PreviewWidget::worker");

                while (1)
                {
                    for (auto seek_req : in_seek)
                    {
                        LOG_DEBUG(logger, "Received seek request, seek_id = {}, cursor = {}", seek_req.first, seek_req.second.count());

                        // Drop all but the latest request for best latency
                        while (!in_seek.empty())
                        {
                            in_seek >> seek_req;
                            LOG_DEBUG(logger, "Have newer seek request, seek_id = {}, cursor = {}", seek_req.first, seek_req.second.count());
                        }

                        const auto &[id, position] = seek_req;
                        composer.seek(position);

                        while (in_seek.empty())
                        {
                            auto *frame = composer.next_frame(AVMEDIA_TYPE_VIDEO);
                            LOG_DEBUG(logger, "Frame ready, pts = {}", frame->pts);
                            out_frames << PreviewFrame{id, frame};
                            LOG_DEBUG(logger, "Frame sent, pts = {}", frame->pts);
                        }
                    }
                }
            }};
        } _preview;

        struct CbUserData
        {
            GLuint vbo, ebo;
            GLuint shader;
            GLuint texture;
            ImVec2 win_pos;
            ImVec2 win_size;
            ImVec2 vp_size;
            ImVec2 img_size;

            GLuint uniform_image;
            GLuint uniform_screen_size;
            GLuint uniform_image_size;
        };

        CbUserData _cb_user;
    };
}

