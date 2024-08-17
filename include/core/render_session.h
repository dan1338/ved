#pragma once

#include "core/event.h"
#include "core/video_composer.h"
#include <thread>

namespace core
{
    class RenderSession
    {
    public:
        RenderSession(core::Timeline &timeline, WorkspaceProperties props);
        
        core::Event<AVFrame*> frame_ready_event;
        core::Event<> finished_event;

    private:
        core::VideoComposer _composer;
    };
}
