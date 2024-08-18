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
#include <variant>

namespace ui
{
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
            Preview(core::Timeline &timeline, const core::WorkspaceProperties &props):
                composer(timeline, props)
            {
            }

            ~Preview()
            {
                in_seek.close();
                in_track_events.close();

                while (!out_frames.empty())
                {
                    PreviewFrame frame;
                    out_frames >> frame;
                    
                    av_frame_unref(frame.second);
                }

                out_frames.close();

                thread.join();
            }

            core::VideoComposer composer;
            AVFrame *last_frame{nullptr};
            std::optional<core::timestamp> last_frame_display_time;
            uint64_t seek_id{0};

            using SeekRequest = std::pair<uint64_t, core::timestamp>;
            msd::channel<SeekRequest> in_seek;
            
            using PreviewFrame = std::pair<uint64_t, AVFrame*>;
            msd::channel<PreviewFrame> out_frames{1};

            struct TrackRemoved { core::Timeline::TrackID id; };
            struct TrackAdded { std::unique_ptr<core::Timeline::Track> track; };
            struct TrackModified { std::unique_ptr<core::Timeline::Track> track; };
            using TrackEvent = std::variant<TrackRemoved, TrackAdded, TrackModified>;
            msd::channel<TrackEvent> in_track_events;

            std::thread thread{[this](){
                auto logger = logging::get_logger("PreviewWidget::worker");

                for (auto seek_req : in_seek)
                {
                    LOG_DEBUG(logger, "Received seek request, seek_id = {}, cursor = {}", seek_req.first, seek_req.second.count());

                    // Drop all but the latest request for best latency
                    while (!in_seek.empty())
                    {
                        in_seek >> seek_req;
                        LOG_DEBUG(logger, "Have newer seek request, seek_id = {}, cursor = {}", seek_req.first, seek_req.second.count());
                    }

                    const auto &[seek_id, position] = seek_req;
                    composer.seek(position);

                    while (in_seek.empty() && !in_seek.closed())
                    {
                        while (!in_track_events.empty())
                        {
                            TrackEvent track_event;
                            in_track_events >> track_event;

                            if (const auto &event_removed = std::get_if<TrackRemoved>(&track_event))
                            {
                                composer.remove_track(event_removed->id);
                            }
                            else if (const auto &event_added = std::get_if<TrackAdded>(&track_event))
                            {
                                composer.update_track(*event_added->track);
                            }
                            else if (const auto &event_modified = std::get_if<TrackModified>(&track_event))
                            {
                                composer.update_track(*event_modified->track);
                            }
                        }

                        auto *frame = composer.next_frame(AVMEDIA_TYPE_VIDEO);
                        LOG_DEBUG(logger, "Frame ready, pts = {}", frame->pts);
                        out_frames << PreviewFrame{seek_id, frame};
                        LOG_DEBUG(logger, "Frame sent, pts = {}", frame->pts);
                    }
                }

                LOG_INFO(logger, "Stopping thread");
            }};
        };

        std::unique_ptr<Preview> _preview;

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

