#include "core/sync_media_source.h"
#include "ffmpeg/media_source.h"
#include "logging.h"

#include "charls/charls.h"

#include <unordered_map>

static auto logger = logging::get_logger("SyncMediaSource");

using FrameCache = std::unordered_map<core::timestamp::rep, AVFrame*>;
static std::unordered_map<std::string, FrameCache> file_caches;

namespace core
{
    static constexpr auto seek_ahead_threshold = 3s;

    SyncMediaSource::SyncMediaSource(core::MediaFile file):
        _file(std::move(file)),
        _raw_source(ffmpeg::open_media_source(_file))
    {
        if (file_caches.find(_file.path) == file_caches.end())
            file_caches[_file.path] = {};
    }

    AVFrame *SyncMediaSource::frame_at(core::timestamp ts)
    {
        LOG_DEBUG(logger, "frame_at, ts = {}s", ts / 1.0s);

        // Single image handling
        // Only the first frame contains the image, if we set ts to 0
        // we'd either fetch it anew if necessary, or grab one from the cache
        if (_file.type == core::MediaFile::STATIC_IMAGE)
        {
            ts = 0s;
        }

        auto &cache = file_caches.at(_file.path);

        if (auto it = cache.find(ts.count()); it != cache.end())
        {
            LOG_DEBUG(logger, "returning cached frame");
            return av_frame_clone(it->second);
        }

        // Because MediaSource only allows for fetching next_frame
        // we might want to create a new one a seek closer to desired timestamp
        if (ts <= _last_ts || ts > _last_ts + seek_ahead_threshold)
        {
            LOG_DEBUG(logger, "reconstruct and seek");
            _raw_source = ffmpeg::open_media_source(_file);
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

        if (frame)
            cache[ts.count()] = av_frame_clone(frame);

        return frame;
    }
}

