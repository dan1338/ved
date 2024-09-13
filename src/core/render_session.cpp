#include "core/render_session.h"

#include "core/application.h"
#include "ffmpeg/media_sink.h"
#include "logging.h"

namespace core
{
    static auto logger = logging::get_logger("RenderSession");

    static WorkspaceProperties props_from_render_settings(const RenderSettings &settings)
    {
        return {
            settings.video.width,
            settings.video.height,
            settings.video.fps,
        };
    }

    RenderSession::RenderSession(core::Timeline &timeline, RenderSettings settings):
        _composer(timeline, props_from_render_settings(settings))
    {
        _sink = ffmpeg::open_media_sink(settings.output_path, {settings.video, {}});

        if (_sink == nullptr)
        {
            LOG_ERROR(logger, "Failed to open media sink, path = {}, video_codec = {}", settings.output_path, avcodec_get_name(settings.video.codec->id));
            throw std::exception();
        }

        _thread = std::thread{[this, &timeline] {
            while (1)
            {
                AVFrame *frame = _composer.next_frame(AVMEDIA_TYPE_VIDEO);

                if (frame == nullptr)
                {
                    LOG_DEBUG(logger, "No more frames available");
                    break;
                }

                if (core::timestamp{frame->pts} >= timeline.get_duration())
                {
                    LOG_DEBUG(logger, "Reached the end of timeline");
                    break;
                }

                frame_ready_event.notify(frame);
                _sink->write_frame(AVMEDIA_TYPE_VIDEO, frame);
            }

            finished_event.notify();
        }};
    }

    RenderSession::~RenderSession()
    {
        _sink.reset();
        _thread.join();
    }
}
