#include "codec/codec.h"

namespace codec
{
    Codec avc = {
        .id = AV_CODEC_ID_H264,
        .name = "avc/h264",
        .load_name = "libx264",
        .supported_exts = { "mp4" },
        .params = {
            {
                "preset",
                {
                    "ultrafast",
                    "superfast",
                    "veryfast",
                    "faster",
                    "fast",
                    "medium",
                    "slow",
                    "slower",
                    "veryslow"
                }
            },
            {
                "tune",
                {
                    "film",
                    "animation",
                    "grain",
                    "stillimage",
                    "fastdecode",
                    "zerolatency"
                }
            },
        }
    };
}

