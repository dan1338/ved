#include "ffmpeg/media_source.h"
#include "core/media_source.h"

#include "fmt/format.h"

#include <stdexcept>
#include <string>
#include <vector>
#include <queue>
#include <memory>

#include "logging.h"

static auto logger = logging::get_logger("MediaSource_ffmpeg");

namespace ffmpeg
{
    static char err_buffer[256];

#define make_errstr(err) av_make_error_string(err_buffer, sizeof err_buffer, err)

    struct Stream
    {
        AVMediaType type;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;
        std::queue<AVFrame*> frame_queue;

        Stream(AVStream *stream):
            type(stream->codecpar->codec_type)
        {
            const auto &codecpar = stream->codecpar;
            codec = avcodec_find_decoder(codecpar->codec_id);
            codec_ctx = avcodec_alloc_context3(codec);

            LOG_DEBUG(logger, "Using codec {} ({})", codec->name, codec->long_name);

            avcodec_parameters_to_context(codec_ctx, codecpar);

            if (int err = avcodec_open2(codec_ctx, codec, nullptr) != 0)
            {
                LOG_CRITICAL(logger, "Cannot open codec, {}", make_errstr(err));
                throw fmt::system_error(err, "avcodec_open2");
            }
        }

        ~Stream()
        {
            while (!frame_queue.empty())
            {
                auto *frame = frame_queue.front();
                av_frame_free(&frame);

                frame_queue.pop();
            }

            avcodec_free_context(&codec_ctx);
        }

        AVFrame* pop_frame()
        {
            if (frame_queue.empty())
                return nullptr;

            auto *frame = frame_queue.back();
            frame_queue.pop();

            return frame;
        }
    };

    using StreamPtr = std::unique_ptr<Stream>;

    class MediaSource : public core::MediaSource
    {
    public:
        MediaSource(core::MediaFile file):
            _file(std::move(file)),
            _format_ctx(nullptr)
        {
            LOG_DEBUG(logger, "Opening {}", _file.path);

            int err = avformat_open_input(&_format_ctx, _file.path.c_str(), nullptr, nullptr);

            if (err != 0)
            {
                avformat_free_context(_format_ctx);
                throw fmt::system_error(err, "avformat_open_input : {}", make_errstr(err));
            }

            avformat_find_stream_info(_format_ctx, nullptr);

            _streams.reserve(_format_ctx->nb_streams);

            for (size_t i = 0; i < _format_ctx->nb_streams; i++)
            {
                auto stream = std::make_unique<Stream>(_format_ctx->streams[i]);

                // Save the first stream index of each kind
                if (stream->type == AVMEDIA_TYPE_AUDIO && _audio_stream == -1)
                {
                    LOG_DEBUG(logger, "Found audio, stream index = {}", i);
                    _audio_stream = i;
                }
                if (stream->type == AVMEDIA_TYPE_VIDEO && _video_stream == -1)
                {
                    LOG_DEBUG(logger, "Found video, stream index = {}", i);
                    _video_stream = i;
                }

                _streams.push_back(std::move(stream));
            }

            _packet = av_packet_alloc();
        }

        ~MediaSource() override
        {
            LOG_DEBUG(logger, "Closing {}", _file.path);

            avformat_close_input(&_format_ctx);
            av_packet_free(&_packet);
        }

        std::string get_name() override
        {
            return _file.path;
        }

        bool seek(core::timestamp position) override
        {
            const auto tb_offset = core::time_cast<core::duration<1, AV_TIME_BASE>>(position).count();

            LOG_DEBUG(logger, "Seek to {}s, tb_offset = {}", position / 1.0s, tb_offset);

            int err = avformat_seek_file(_format_ctx, -1, 0, tb_offset, tb_offset, 0);

            return err >= 0;
        }

        bool seek(int64_t byte_offset) override
        {
            //avio_tell(_format_ctx);

            int err = av_seek_frame(_format_ctx, -1, byte_offset, AVSEEK_FLAG_BYTE);

            return err >= 0;
        }

        AVFrame* next_frame(AVMediaType frame_type) override
        {
            Stream* wanted_stream;

            if (frame_type == AVMEDIA_TYPE_AUDIO)
                wanted_stream = get_audio_stream();
            else if (frame_type == AVMEDIA_TYPE_VIDEO)
                wanted_stream = get_video_stream();
            else
                throw std::runtime_error("Unsupported media type");

            // Maybe we already have it?
            if (auto *frame = wanted_stream->pop_frame())
                return frame;

            AVFrame *frame = av_frame_alloc();

            LOG_TRACE_L3(logger, "Begin next_frame");

            while (1)
            {
                int err = av_read_frame(_format_ctx, _packet);

                if (err == AVERROR_EOF)
                {
                    LOG_DEBUG(logger, "End of file {}", _file.path);
                    break;
                }

                LOG_TRACE_L1(logger, "Read packet, pts = {}, size = {}", _packet->pts, _packet->size);

                auto *stream = _streams[_packet->stream_index].get();
                const auto tb = _format_ctx->streams[_packet->stream_index]->time_base;

                avcodec_send_packet(stream->codec_ctx, _packet);
                av_packet_unref(_packet);

                err = avcodec_receive_frame(stream->codec_ctx, frame);

                if (err == 0)
                {
                    // Rewrite pts into timestamp units
                    const auto original_pts = frame->pts;
                    frame->pts = (core::timestamp(1s).count() * frame->pts / tb.den);

                    if (stream == wanted_stream)
                    {
                        LOG_TRACE_L1(logger, "Receive frame, original_pts = {}, new_pts = {}", original_pts, frame->pts);
                        LOG_TRACE_L3(logger, "End next_frame");
                        return frame;
                    }
                    else
                    {
                        stream->frame_queue.push(frame);
                        frame = av_frame_alloc();
                    }
                }
                else if (err != AVERROR(EAGAIN))
                {
                    throw std::runtime_error("avcodec_receive_frame");
                }
            }

            LOG_TRACE_L3(logger, "End next_frame");

            av_packet_unref(_packet);
            av_frame_free(&frame);

            return nullptr;
        }

        bool has_stream(AVMediaType frame_type) override
        {
            if (frame_type == AVMEDIA_TYPE_AUDIO)
            {
                return _audio_stream != -1;
            }
            else if (frame_type == AVMEDIA_TYPE_VIDEO)
            {
                return _video_stream != -1;
            }

            return false;
        }

    private:
        core::MediaFile _file;
        AVFormatContext *_format_ctx;
        AVPacket *_packet;
        std::vector<StreamPtr> _streams;
        int _video_stream{-1};
        int _audio_stream{-1};

        Stream* get_audio_stream() const noexcept
        {
            return _streams[_audio_stream].get();
        }

        Stream* get_video_stream() const noexcept
        {
            return _streams[_video_stream].get();
        }
    };

    std::unique_ptr<core::MediaSource> open_media_source(const core::MediaFile &file)
    {
        try
        {
            return std::make_unique<MediaSource>(file);
        }
        catch (const std::exception &e)
        {
            fmt::println(e.what());

            return nullptr;
        }
    }
}

