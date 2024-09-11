#pragma once

#include "core/media_sink.h"
#include "core/media_file.h"
#include "core/video_properties.h"
#include "codec/codec.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace ffmpeg
{
    struct SinkOptions
    {
        struct VideoStream : public core::VideoProperties
        {
            codec::Codec *codec;
            codec::CodecParams codec_params;
            int bitrate;
            int crf;
        };

        struct AudioStream
        {
            codec::Codec *codec;
            codec::CodecParams codec_params;
            int sample_rate;
            int channels;
        };

        std::optional<VideoStream> video_desc;
        std::optional<AudioStream> audio_desc;
    };

    std::unique_ptr<core::MediaSink> open_media_sink(const std::string &path, const SinkOptions &opt);
}

