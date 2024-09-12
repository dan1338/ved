#include "codec/codec.h"

namespace codec
{
    Codec vp8 = {
        .id = AV_CODEC_ID_VP8,
        .name = "vp8",
        .load_name = { "libvpx" },
        .supported_exts = { "mp4", "mkv", "webm" },
        .params = {}
    };
}

