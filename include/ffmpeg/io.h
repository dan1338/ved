#pragma once

#include <string>

#include "core/media_file.h"

namespace ffmpeg
{
    namespace io
    {
        core::MediaFile open_file(const std::string &path);
    }
}

