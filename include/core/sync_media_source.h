#pragma once

#include "core/media_source.h"
#include "core/media_file.h"

#include <string>
#include <memory>

namespace core
{
    class SyncMediaSource
    {
    public:
        SyncMediaSource(core::MediaFile file);

        AVFrame *frame_at(core::timestamp ts);

    private:
        core::MediaFile _file;
        std::unique_ptr<core::MediaSource> _raw_source;
        core::timestamp _last_ts{0s};
    };
}

