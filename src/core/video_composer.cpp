#include "core/video_composer.h"
#include "logging.h"
#include <algorithm>

static auto logger = logging::get_logger("VideoComposer");

namespace core
{
    static std::pair<ClipTransform, ClipTransform> find_current_clip_transforms(const Timeline::Clip &clip, const core::timestamp ts)
    {
        assert(!clip.transforms.empty());

        const auto rel_ts = ts - clip.position;
        auto it = clip.transforms.begin();

        // Skip until we're after the transform timestamp
        while (it != clip.transforms.end() && rel_ts < it->rel_position)
            it = std::next(it);

        // Ran past the end, go back to last element
        if (it == clip.transforms.end())
            it = std::prev(it);

        // Take next or keep it2 same as it
        auto it2 = std::next(it) != clip.transforms.end()
            ? std::next(it)
            : it;

        return {*it, *it2};
    }

    static void blit_pixels(AVFrame *dst_frame, AVFrame *src_frame, int dst_x, int dst_y)
    {
        const auto dst_stride = dst_frame->linesize[0];
        const auto src_stride = src_frame->linesize[0];
        unsigned char *dst = dst_frame->data[0];
        const unsigned char *src = src_frame->data[0];

        for (int i = 0; i < src_frame->height; i++)
        {
            for (int j = 0; j < src_frame->width; j++)
            {
                int dst_i = i + dst_y;
                int dst_j = j + dst_x;

                if (dst_i < 0 || dst_i >= dst_frame->height)
                    continue;

                if (dst_j < 0 || dst_j >= dst_frame->width)
                    continue;

                dst[dst_i * dst_stride + dst_j * 3 + 0] = src[i * src_stride + j * 3 + 0];
                dst[dst_i * dst_stride + dst_j * 3 + 1] = src[i * src_stride + j * 3 + 1];
                dst[dst_i * dst_stride + dst_j * 3 + 2] = src[i * src_stride + j * 3 + 2];
            }
        }
    }

    VideoComposer::VideoComposer(core::Timeline &timeline, WorkspaceProperties props):
        _props(std::move(props)),
        _frame_dt(_props.frame_dt())
    {
        seek(0s);

        timeline.foreach_track([this](Timeline::Track &track) {
            add_track(track);

            for (auto &[clip_id, clip] : track.clips)
                add_clip(clip);

            return true;
        });
    }

    void VideoComposer::update_properties(WorkspaceProperties props)
    {
        _props = std::move(props);
        _frame_dt = _props.frame_dt();
    }

    void VideoComposer::update_track(core::Timeline::Track &track)
    {
        LOG_DEBUG(logger, "update track, num_clips = {}", track.clips.size());

        if (_tracks.find(track.id) != _tracks.end())
            rm_track(track.id);

        add_track(track);

        std::vector<core::Timeline::ClipID> unused_clips;

        // Find clips which are no longer referenced
        for (const auto &source : _sources)
        {
            const auto track_has_clip = [&](auto &it){
                const auto &track = it.second;
                return track.clips.find(source.first) != track.clips.end();
            };

            if (std::none_of(_tracks.begin(), _tracks.end(), track_has_clip))
                unused_clips.push_back(source.first);
        }

        // Remove the sources used for fetching those clips
        for (const auto clip_id : unused_clips)
        {
            LOG_DEBUG(logger, "Removing unused clip source, clip_id = {}", clip_id);
            rm_clip(clip_id);
        }

        // Add sources to newly referenced clips
        for (auto &[clip_id, clip] : track.clips)
        {
            if (_sources.find(clip.id) == _sources.end())
                add_clip(clip);
        }
    }

    void VideoComposer::remove_track(core::Timeline::TrackID id)
    {
        if (_tracks.find(id) != _tracks.end())
            _tracks.erase(id);
    }

    std::string VideoComposer::get_name()
    {
        return {};
    }

    bool VideoComposer::seek(core::timestamp position)
    {
        // The timestamps used with SyncMediaSource have to be a multiple of frame_dt
        const auto dt_aligned_position = position - (position % _frame_dt);

        LOG_DEBUG(logger, "Seek, asked = {}s, actual = {}", position / 1.0s, dt_aligned_position / 1.0s);

        _composition = std::make_unique<Composition>();
        _composition->start_position = dt_aligned_position;
        _composition->last_position = dt_aligned_position;

        return true;
    }

    bool VideoComposer::seek(int64_t byte_offset)
    {
        return false;
    }

    AVFrame* VideoComposer::next_frame(AVMediaType frame_type)
    {
        auto &ts = _composition->last_position;

        AVFrame *out_frame = av_frame_alloc();
        out_frame->pts = ts.count();
        out_frame->duration = _frame_dt.count();

        out_frame->width = _props.video.width;
        out_frame->height = _props.video.height;
        out_frame->format = AVPixelFormat::AV_PIX_FMT_RGB24;

        if (av_frame_get_buffer(out_frame, 0) != 0)
            throw std::runtime_error("av_frame_get_buffer @ render");

        // Zero out frame
        memset(out_frame->data[0], 0, out_frame->linesize[0] * out_frame->height);

        LOG_DEBUG(logger, "Next frame, ts = {}s", ts / 1.0s);
        LOG_TRACE_L3(logger, "Begin compose");

        for (auto &[track_id, track] : _tracks)
        {
            auto clip_id = track.clip_at(ts);
            
            if (!clip_id.has_value()) // No clip here
                continue;

            auto &clip = track.clips[*clip_id];
            auto &source = _sources.at(clip.id);

            AVFrame *clip_frame = source.frame_at(ts - clip.position + clip.start_time);

            if (!clip_frame)
            {
                LOG_DEBUG(logger, "No frame found");
                continue;
            }

            // Only apply passed transform for now
            // TODO: Implement transform interpolation with various functions
            const auto [xform1, xform2] = find_current_clip_transforms(clip, ts);

            const auto target_x = (int)(out_frame->width * xform1.translate_x);
            const auto target_y = (int)(out_frame->height * xform1.translate_y);
            const auto target_width = (int)(out_frame->width * xform1.scale_x);
            const auto target_height = (int)(out_frame->height * xform1.scale_y);

            auto &frame_converter = _frame_converters.at(track.id);

            AVFrame *tmp_frame = frame_converter.convert(clip_frame, target_width, target_height);
            av_frame_unref(clip_frame);

            blit_pixels(out_frame, tmp_frame, target_x, target_y);
            av_frame_unref(tmp_frame);
        }

        LOG_TRACE_L3(logger, "End compose");

        _composition->frames.push_back(out_frame); // TODO: remove this
        ts += _frame_dt;

        return out_frame;
    }

    bool VideoComposer::has_stream(AVMediaType frame_type)
    {
        return (frame_type == AVMEDIA_TYPE_AUDIO || frame_type == AVMEDIA_TYPE_VIDEO);
    }

    void VideoComposer::add_clip(Timeline::Clip &clip)
    {
        // We only support forward reading of frames
        // check if clip is reachable
        if (clip.end_position() < _composition->start_position)
            return;

        _sources.emplace(clip.id, clip.file);
    }

    void VideoComposer::add_track(Timeline::Track &track)
    {
        LOG_TRACE_L1(logger, "add track, id = {}", track.id);

        _tracks.emplace(track.id, track);

        if (_frame_converters.find(track.id) == _frame_converters.end())
            _frame_converters.emplace(track.id, ffmpeg::FrameConverter(AVPixelFormat::AV_PIX_FMT_RGB24));
    }

    void VideoComposer::rm_track(Timeline::TrackID track_id)
    {
        LOG_TRACE_L1(logger, "rm track, id = {}", track_id);

        _tracks.erase(track_id);

        if (_frame_converters.find(track_id) != _frame_converters.end())
            _frame_converters.erase(track_id);
    }

    void VideoComposer::rm_clip(Timeline::ClipID clip_id)
    {
        LOG_TRACE_L1(logger, "rm clip, id = {}", clip_id);

        _sources.erase(clip_id);
    }
}

