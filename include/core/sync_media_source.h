#pragma once

#include "core/media_source.h"

#include <string>
#include <memory>

namespace core
{
    class SyncMediaSource
    {
    public:
        SyncMediaSource(std::string path);

        AVFrame *frame_at(core::timestamp ts);

    private:
        std::string _path;
        std::unique_ptr<core::MediaSource> _raw_source;
        core::timestamp _last_ts{0s};
    };
}

