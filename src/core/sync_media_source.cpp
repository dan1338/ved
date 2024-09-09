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

    AVFrame *SyncMediaSource::frame_at(core::timestamp req_ts)
    {
        core::timestamp ts = req_ts;

        LOG_DEBUG(logger, "frame_at, ts = {}s, lreq = {}s, lret = {}s", req_ts / 1.0s, _last_req_ts / 1.0s, _last_ret_ts / 1.0s);

        // Single image handling
        // Only the first frame contains the image, if we set ts to 0
        // we'd either fetch it anew if necessary, or grab one from the cache
        if (_file.type == core::MediaFile::STATIC_IMAGE)
        {
            ts = 0s;
        }

        // If newly requested ts is still less than last returned frame
        // Means we want to keep displaying the last frame
        if (ts <= _last_ret_ts && ts > _last_req_ts)
        {
            LOG_DEBUG(logger, "repeat last frame, ts = {}s", ts / 1.0s);
            ts = _last_req_ts;
        }

        auto &cache = file_caches.at(_file.path);

        // Return frame from cache early if present
        if (auto it = cache.find(ts.count()); it != cache.end())
        {
            const auto *frame = it->second;

            LOG_DEBUG(logger, "return cached frame, ts = {}s", core::timestamp{frame->pts} / 1.0s);
            _last_req_ts = req_ts;
            _last_ret_ts = core::timestamp{frame->pts};

            return av_frame_clone(frame);
        }

        // Because MediaSource only allows for fetching next_frame
        // we might want to create a new one a seek closer to desired timestamp
        if (ts <= _last_req_ts || ts > _last_req_ts + seek_ahead_threshold)
        {
            LOG_DEBUG(logger, "reconstruct and seek");
            _raw_source = ffmpeg::open_media_source(_file);
            _raw_source->seek(ts);
        }

        const auto [frame, fetch_count] = skip_frames_until(ts);

        LOG_DEBUG(logger, "return frame_at, fetch_count = {}, ts = {}", fetch_count, core::timestamp{frame->pts} / 1.0s);

        _last_req_ts = req_ts;
        _last_ret_ts = core::timestamp{frame->pts};

        if (frame)
            cache[ts.count()] = av_frame_clone(frame);

        return frame;
    }

    std::pair<AVFrame*, int> SyncMediaSource::skip_frames_until(core::timestamp ts)
    {
        AVFrame *frame{nullptr};
        core::timestamp frame_ts;
        size_t fetch_count = 0;

        // Get next frame which aligns with current timestamp
        do
        {
            if (frame)
                av_frame_unref(frame);

            frame = _raw_source->next_frame(AVMEDIA_TYPE_VIDEO);
            ++fetch_count;

            if (!frame) // Early EOF
            {
                return {nullptr, fetch_count};
            }

            frame_ts = core::timestamp(frame->pts);
        }
        while (frame_ts < ts);

        return {frame, fetch_count};
    }
}

