#pragma once 

#include "core/media_source.h"
#include "core/media_file.h"

#include <memory>

namespace ffmpeg
{
    std::unique_ptr<core::MediaSource> open_media_source(const core::MediaFile &file);
}

