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

    SyncMediaSource::SyncMediaSource(std::string path):
        _path(std::move(path)),
        _raw_source(ffmpeg::open_media_source(_path))
    {
        if (file_caches.find(_path) == file_caches.end())
            file_caches[_path] = {};
    }

    AVFrame *SyncMediaSource::frame_at(core::timestamp ts)
    {
        LOG_DEBUG(logger, "frame_at, ts = {}s", ts / 1.0s);

        auto &cache = file_caches.at(_path);

        if (auto it = cache.find(ts.count()); it != cache.end())
        {
            LOG_DEBUG(logger, "returning cached frame");
            return av_frame_clone(it->second);
        }

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

        if (frame)
            cache[ts.count()] = av_frame_clone(frame);

        return frame;
    }
}

