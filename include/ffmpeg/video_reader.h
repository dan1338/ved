#pragma once

#include <queue>
#include <stdexcept>

#include "fmt/base.h"
#include "fmt/format.h"
#include "headers.h"

#include "media_stream.h"
#include "media_file_handle.h"

#include "core/media_file.h"
#include "core/time.h"

namespace ffmpeg
{
    class VideoReader
    {
    public:
        VideoReader(const core::MediaFile &file):
            _file_handle(file),
            _frame(av_frame_alloc()),
            _packet(av_packet_alloc())
        {
            auto video_indices = _file_handle.streams_of_type(AVMediaType::AVMEDIA_TYPE_VIDEO);

            if (!video_indices.empty()) {
                _video_stream = &_file_handle.streams[video_indices[0]];

                if (video_indices.size() > 1) {
                    fmt::println("Multiple video streams found in {}, using first", _file_handle.file.path);
                }
            }
        }

        ~VideoReader()
        {
            av_frame_free(&_frame);
            av_packet_free(&_packet);
        }

        void seek(core::timestamp position)
        {
            auto format_ctx = _file_handle.format_ctx;

            const auto tb = _video_stream->handle->time_base;
            const int64_t target_pts = (position * tb.den / tb.num) / 1s;

            av_seek_frame(format_ctx, _video_stream->handle->index, target_pts, 0);
        }

        AVFrame *next_frame()
        {
            int err;
            bool eof = false;
            auto format_ctx = _file_handle.format_ctx;
            auto codec_ctx = _video_stream->codec_ctx;

            int tries_left = 30;

            while (tries_left--)
            {
                err = av_read_frame(format_ctx, _packet);

                if (err != 0)
                {
                    // TODO: reset stream if we got an EOF want to keep using it
                    eof = true;
                    avcodec_send_packet(codec_ctx, nullptr); // Flush decoder

                    if (avcodec_receive_frame(codec_ctx, _frame) == 0) {
                        _last_pts = _frame->pts;
                        return _frame;
                    }

                    break;
                }

                if (_packet->stream_index == _video_stream->handle->index)
                {
                    err = avcodec_send_packet(codec_ctx, _packet);
                    av_packet_unref(_packet);

                    if (err != 0 && err != AVERROR(EAGAIN))
                    {
                        char buf[256];
                        throw fmt::system_error(0, "avcodec_send_packet: {}", av_make_error_string(buf, sizeof buf, err));
                    }

                    err = avcodec_receive_frame(codec_ctx, _frame);

                    if (err == 0)
                    {
                        _last_pts = _frame->pts;
                        return _frame;
                    }
                    else if (err != AVERROR(EAGAIN))
                    {
                        char buf[256];
                        throw fmt::system_error(0, "avcodec_receive_frame: {}", av_make_error_string(buf, sizeof buf, err));
                    }
                }
            }

            return nullptr;
        }

        AVFrame *frame_at(core::timestamp position)
        {
            auto format_ctx = _file_handle.format_ctx;

            const auto tb = _video_stream->handle->time_base;
            const int64_t target_pts = (position * tb.den / tb.num) / 1s;

            if (target_pts < _last_pts) {
                int ok = av_seek_frame(format_ctx, _video_stream->handle->index, target_pts, AVSEEK_FLAG_BACKWARD);
                fmt::println("SEEK");

                if (ok < 0) {
                    throw std::runtime_error("seeking failed");
                }
            }

            while (next_frame()) {
                if (_frame->pts >= target_pts) {
                    return _frame;
                }
                av_frame_unref(_frame);
            }

            return nullptr;
        }

    private:
        MediaFileHandle _file_handle;
        const MediaStream *_video_stream;

        AVFrame *_frame;
        AVPacket *_packet;

        int64_t _last_pts{-1};
    };
}

