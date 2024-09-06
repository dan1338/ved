#pragma once

#include "core/event.h"
#include "core/media_sink.h"
#include "core/video_composer.h"
#include <thread>

namespace core
{
    struct RenderSettings
    {
        std::string output_path;

        struct {
            int fps;
            int width;
            int height;
            int crf;
            int bitrate;
        } video;
    };

    class RenderSession
    {
    public:
        RenderSession(core::Timeline &timeline, RenderSettings settings);
        
        core::Event<AVFrame*> frame_ready_event;
        core::Event<> finished_event;

    private:
        core::VideoComposer _composer;
        std::unique_ptr<core::MediaSink> _sink;
        std::thread _thread;
    };
}
