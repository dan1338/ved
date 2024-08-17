#pragma once

#include "imgui/imgui.h"

#include "core/workspace.h"
#include "core/video_composer.h"
#include "core/render_session.h"
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
    class MainWindow;

    class PreviewWorker
    {
    public:
        PreviewWorker();
        virtual ~PreviewWorker();

        AVFrame *last_frame{nullptr};
        uint64_t seek_id{0};
        std::optional<core::timestamp> last_frame_display_time;

        using SeekRequest = std::pair<uint64_t, core::timestamp>;
        msd::channel<SeekRequest> in_seek;
        
        using PreviewFrame = std::pair<uint64_t, AVFrame*>;
        msd::channel<PreviewFrame> out_frames{1};

        struct TrackRemoved { core::Timeline::TrackID id; };
        struct TrackAdded { std::unique_ptr<core::Timeline::Track> track; };
        struct TrackModified { std::unique_ptr<core::Timeline::Track> track; };
        using TrackEvent = std::variant<TrackRemoved, TrackAdded, TrackModified>;
        msd::channel<TrackEvent> in_track_events;

    protected:
        std::thread _thread;

        virtual void run() = 0;
    };

    class LivePreviewWorker : public PreviewWorker
    {
    public:
        LivePreviewWorker(core::Timeline &timeline, const core::WorkspaceProperties &props);
        
    private:
        core::VideoComposer _composer;

        void run() override;
    };

    class RenderPreviewWorker : public PreviewWorker
    {
    public:
        RenderPreviewWorker(core::RenderSession &render_session);

    private:
        core::RenderSession &_render_session;
        msd::channel<AVFrame*> _ready_frames;

        void run() override;
    };

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
        
        std::unique_ptr<PreviewWorker> _preview;

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

