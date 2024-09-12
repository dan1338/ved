#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "ffmpeg/headers.h"

namespace codec
{
    using CodecParamsDefinition = std::unordered_map<std::string, std::vector<std::string>>;
    using CodecParams = std::unordered_map<std::string, std::string>;

    struct Codec
    {
        // FFMPEG identifier
        AVCodecID id;

        // Name to be displayed
        std::string name;

        // Require the encoder to be loaded by this name instead of codec id
        std::optional<std::string> load_name;
        
        // List of supported media containers / extensions
        std::vector<std::string> supported_exts;

        // List of parameters associated with allowed values
        CodecParamsDefinition params;
    };

    extern Codec avc;
    extern Codec vp8;
    extern Codec vp9;
}

