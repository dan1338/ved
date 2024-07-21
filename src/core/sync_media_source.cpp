#include "core/sync_media_source.h"
#include "ffmpeg/media_source.h"

#include "logging.h"

static constexpr auto seek_ahead_threshold = 3s;

static auto logger = logging::get_logger("SyncMediaSource");

namespace core
{
    SyncMediaSource::SyncMediaSource(std::string path):
        _path(std::move(path)),
        _raw_source(ffmpeg::open_media_source(_path))
    {
    }

    AVFrame *SyncMediaSource::frame_at(core::timestamp ts)
    {
        LOG_DEBUG(logger, "frame_at, ts = {}s", ts / 1.0s);

        if (ts <= _last_ts || ts > _last_ts + seek_ahead_threshold)
        {
            LOG_DEBUG(logger, "reconstruct and seek");
            _raw_source = ffmpeg::open_media_source(_path);
            _raw_source->seek(ts);
        }

        AVFrame *frame{nullptr};
        core::timestamp frame_ts;

        // Get frame which aligns with current timestamp
        do
        {
            if (frame)
                av_frame_unref(frame);

            frame = _raw_source->next_frame(AVMEDIA_TYPE_VIDEO);

            if (!frame) // Early EOF
            {
                break;
            }

            frame_ts = core::timestamp(frame->pts);
        }
        while (frame_ts < ts);

        _last_ts = ts;

        return frame;
    }
}

