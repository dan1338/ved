#pragma once

#include "imgui/imgui.h"

#include "core/workspace.h"
#include "core/video_composer.h"
#include "ffmpeg/frame_converter.h"
#include "ui/widget_ids.h"

#include "msd/channel.hpp"

#include "shaders.h"

#include <thread>
#include <atomic>
#include <memory>

namespace ui
{
    class PreviewWidget
    {
    public:
        PreviewWidget(core::Workspace &workspace);
        ~PreviewWidget();

        void show();

    private:
        static constexpr auto *_widget_name = ui::widget_ids::preview;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::Workspace &_workspace;
        
        struct Preview
        {
            core::VideoComposer composer;
            AVFrame *last_frame{nullptr};
            uint64_t seek_id{0};

            core::timestamp presentation_origin;
            core::timestamp presentation_time;

            using SeekRequest = std::pair<uint64_t, core::timestamp>;
            msd::channel<SeekRequest> in_seek;
            
            using PreviewFrame = std::pair<uint64_t, AVFrame*>;
            msd::channel<PreviewFrame> out_frames{10};

            std::thread thread{[this](){
                while (1)
                {
                    for (auto seek_req : in_seek)
                    {
                        // Drop all but the latest request for best latency
                        while (!in_seek.empty())
                            in_seek >> seek_req;

                        const auto &[id, position] = seek_req;
                        composer.seek(position);

                        while (in_seek.empty())
                        {
                            auto *frame = composer.next_frame(AVMEDIA_TYPE_VIDEO);
                            out_frames << PreviewFrame{id, frame};
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

