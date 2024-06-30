#pragma once 

#include "core/media_source.h"

#include <memory>

namespace ffmpeg
{
    std::unique_ptr<core::MediaSource> open_media_source(const std::string &path);
}

