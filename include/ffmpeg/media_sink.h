#pragma once

#include "core/media_sink.h"
#include "core/media_file.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace ffmpeg
{
    struct SinkOptions
    {
        struct VideoStream
        {
            AVCodecID codec_id;
            int fps;
            int width;
            int height;
            int bitrate;
            int crf;
        };

        struct AudioStream
        {
            AVCodecID codec_id;
            int sample_rate;
            int channels;
        };

        std::optional<VideoStream> video_desc;
        std::optional<AudioStream> audio_desc;
    };

    std::unique_ptr<core::MediaSink> open_media_sink(const std::string &path, const SinkOptions &opt);
}
