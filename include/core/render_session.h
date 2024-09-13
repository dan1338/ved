#pragma once

#include "core/event.h"
#include "core/media_sink.h"
#include "core/video_composer.h"
#include "codec/codec.h"
#include "core/video_properties.h"
#include "ffmpeg/media_sink.h"
#include <thread>

namespace core
{
    struct RenderSettings
    {
        std::string output_path;

        ffmpeg::SinkOptions::VideoStream video;
        ffmpeg::SinkOptions::AudioStream audio;
    };

    class RenderSession
    {
    public:
        RenderSession(core::Timeline &timeline, RenderSettings settings);
        ~RenderSession();
        
        core::Event<AVFrame*> frame_ready_event;
        core::Event<> finished_event;

    private:
        core::VideoComposer _composer;
        std::unique_ptr<core::MediaSink> _sink;
        std::thread _thread;
    };
}
