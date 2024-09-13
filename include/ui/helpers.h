#pragma once

#include "core/time.h"
#include "logging.h"

namespace ui
{
    core::timestamp now();
    void handle_playback_control(logging::Logger *logger);
}

