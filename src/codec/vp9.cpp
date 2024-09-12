#include "codec/codec.h"

namespace codec
{
    Codec vp9 = {
        .id = AV_CODEC_ID_VP9,
        .name = "vp9",
        .load_name = { "libvpx-vp9" },
        .supported_exts = { "mp4", "mkv", "webm" },
        .params = {
            {
                "deadline",
                {
                    "realtime",
                    "good",
                    "best",
                }
            },
            {
                "cpu-used",
                {
                    "good",
                    "best",
                }
            },
        }
    };
}

