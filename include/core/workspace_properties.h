#pragma once

#include "core/time.h"

namespace core
{
    struct WorkspaceProperties
    {
        int video_width;
        int video_height;
        int frame_rate;

        core::timestamp frame_dt() const
        {
            return core::timestamp{core::timestamp(1s).count() / frame_rate};
        }
    };
}
