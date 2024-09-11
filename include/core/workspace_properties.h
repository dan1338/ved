#pragma once

#include "core/time.h"
#include "core/video_properties.h"

namespace core
{
    struct WorkspaceProperties
    {
        VideoProperties video;

        core::timestamp frame_dt() const
        {
            return core::timestamp{core::timestamp(1s).count() / video.fps};
        }
    };
}
