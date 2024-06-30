#pragma once

#include <string>
#include <cstdint>

#include "core/time.h"

namespace core
{
    struct MediaFile
    {
        enum Type
        {
            VIDEO,
            AUDIO,
            STATIC_IMAGE,
        } type;

        std::string path;

        core::timestamp duration;
    };
}

