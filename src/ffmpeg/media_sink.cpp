#include "ffmpeg/media_sink.h"

#include "ffmpeg/frame_converter.h"
#include "ffmpeg/headers.h"
#include "logging.h"
#include "magic_enum.hpp"
#include <unordered_map>

namespace ffmpeg
{
    static auto logger = logging::get_logger("MediaSink");

    const AVCodec *find_best_encoder(const enum AVCodecID codec_id)
    {
        const AVCodec *codec = avcodec_find_encoder(codec_id);

        if (codec_id == AV_CODEC_ID_H264)
        {
            codec = avcodec_find_encoder_by_name("libx264");
        }

        return codec;
    }

    class MediaSink : public core::MediaSink
    {
    public:
        MediaSink(const std::string &path, const SinkOptions &opt):
            _path(path),
            _pkt(av_packet_alloc()),
            _frame_converter(AV_PIX_FMT_YUV420P)
        {
            const AVOutputFormat *oformat = av_guess_format(0, path.c_str(), 0);

            if (oformat == nullptr)
            {
                throw std::runtime_error("Cannot find output format");
            }

            _format_ctx = avformat_alloc_context();
            _format_ctx->oformat = oformat;

            if (opt.video_desc.has_value())
            {
                const auto &desc = *opt.video_desc;

                if (const auto *video_codec = find_best_encoder(desc.codec_id))
                {
                    _video_stream = avformat_new_stream(_format_ctx, nullptr);
                    _video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
                    _video_stream->codecpar->codec_id = desc.codec_id;
                    _video_stream->codecpar->format = AV_PIX_FMT_YUV420P;
                    _video_stream->codecpar->width = desc.width;
                    _video_stream->codecpar->height = desc.height;
                    _video_stream->codecpar->bit_rate = 4000000;
                    _video_stream->codecpar->framerate = AVRational{desc.fps, 1};

                    _video_stream->time_base = AVRational{1, 1000};
                    _video_stream->avg_frame_rate = _video_stream->r_frame_rate = AVRational{desc.fps, 1};

                    AVCodecContext *codec_ctx = avcodec_alloc_context3(video_codec);
                    avcodec_parameters_to_context(codec_ctx, _video_stream->codecpar);
                    codec_ctx->time_base = AVRational{1, desc.fps};
                    codec_ctx->gop_size = 12; // Force I frame at least once per 12 frames

                    if (av_opt_set(codec_ctx, "crf", "26", 0) != 0)
                    {
                        LOG_WARNING(logger, "Failed to set codec crf");
                    }

                    if (av_opt_set(codec_ctx, "preset", "normal", 0) != 0)
                    {
                        LOG_WARNING(logger, "Failed to set codec preset");
                    }

                    if (avcodec_open2(codec_ctx, video_codec, nullptr) != 0)
                    {
                        LOG_ERROR(logger, "Cannot open codec, name = {}", video_codec->name);
                        throw std::runtime_error("avcodec_open2");
                    }

                    _stream_encoders.emplace(AVMEDIA_TYPE_VIDEO, codec_ctx);
                }
            }

            if (opt.audio_desc.has_value())
            {
                const auto &desc = *opt.audio_desc;

                if (const auto *audio_codec = find_best_encoder(desc.codec_id))
                {
                    _audio_stream = avformat_new_stream(_format_ctx, nullptr);
                    _audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
                    _audio_stream->codecpar->codec_id = desc.codec_id;
                    _audio_stream->codecpar->format = AV_SAMPLE_FMT_FLTP;
                    _audio_stream->codecpar->bit_rate = 320000;
                    _audio_stream->codecpar->sample_rate = desc.sample_rate;

                    AVCodecContext *codec_ctx = avcodec_alloc_context3(audio_codec);

                    if (avcodec_open2(codec_ctx, audio_codec, nullptr) != 0)
                    {
                        LOG_ERROR(logger, "Cannot open codec, name = {}", audio_codec->name);
                        throw std::runtime_error("avcodec_open2");
                    }

                    _stream_encoders.emplace(AVMEDIA_TYPE_AUDIO, codec_ctx);
                }
            }

            if (avio_open(&_format_ctx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                throw std::runtime_error("Cannot open avio");
            }

            if (avformat_write_header(_format_ctx, nullptr) != 0)
            {
                throw std::runtime_error("Failed to write header");
            }

            LOG_INFO(logger, "Wrote file header");
        }

        std::string get_name() override
        {
            return _path;
        }

        void write_frame(AVMediaType frame_type, AVFrame *frame) override
        {
            AVStream *stream = (frame_type == AVMEDIA_TYPE_VIDEO)
                ? _video_stream
                : _audio_stream;

            auto *encoder = _stream_encoders.at(frame_type);
            bool must_free{false};

            if (frame_type == AVMEDIA_TYPE_VIDEO)
            {
                LOG_INFO(logger, "write_frame, pts = {}", frame->pts);

                frame = _frame_converter.convert(frame, frame->width, frame->height);
                frame->pts = encoder->frame_num;

                LOG_INFO(logger, "convert, pts = {}", frame->pts);

                must_free = true;
            }

            if (avcodec_send_frame(encoder, frame) != 0)
            {
                LOG_ERROR(logger, "Failed to send frame to encoder");
                throw std::runtime_error("avcodec_send_frame");
            }

            while (avcodec_receive_packet(encoder, _pkt) == 0)
            {
                _pkt->pts = rescale_timestamp(_pkt->pts, stream, encoder);
                _pkt->dts = rescale_timestamp(_pkt->dts, stream, encoder);

                LOG_DEBUG(logger, "Got packet, size = {}, pts = {}, dts = {}", _pkt->size, _pkt->pts, _pkt->dts);

                if (av_interleaved_write_frame(_format_ctx, _pkt) != 0)
                {
                    LOG_ERROR(logger, "Failed to write packet");
                    throw std::runtime_error("av_interleaved_write_frame");
                }
            }

            if (must_free)
            {
                av_frame_unref(frame);
            }
        }

        ~MediaSink()
        {
            LOG_INFO(logger, "Closing media sink, path = {}", _path);

            for (auto &[frame_type, encoder] : _stream_encoders)
            {
                AVStream *stream = (frame_type == AVMEDIA_TYPE_VIDEO)
                    ? _video_stream
                    : _audio_stream;

                LOG_INFO(logger, "Flushing stream, type = {}", magic_enum::enum_name<AVMediaType>(frame_type));

                avcodec_send_frame(encoder, nullptr);

                while (avcodec_receive_packet(encoder, _pkt) == 0)
                {
                    _pkt->pts = rescale_timestamp(_pkt->pts, stream, encoder);
                    _pkt->dts = rescale_timestamp(_pkt->dts, stream, encoder);

                    LOG_DEBUG(logger, "Got packet, size = {}, pts = {}, dts = {}", _pkt->size, _pkt->pts, _pkt->dts);

                    if (av_interleaved_write_frame(_format_ctx, _pkt) != 0)
                    {
                        LOG_WARNING(logger, "Failed to write trailing packet");
                        break;
                    }
                }

                av_interleaved_write_frame(_format_ctx, nullptr);
                av_write_trailer(_format_ctx);

                avformat_free_context(_format_ctx);
            }
        }

    private:
        std::string _path;
        AVFormatContext *_format_ctx;
        AVPacket *_pkt;
        AVStream *_video_stream{nullptr};
        AVStream *_audio_stream{nullptr};
        ffmpeg::FrameConverter _frame_converter; // Some codecs like MPEG4, only support YUV pix_fmt
        std::unordered_map<AVMediaType, AVCodecContext*> _stream_encoders;

        int64_t rescale_timestamp(int64_t ts, AVStream *stream, AVCodecContext *codec_ctx)
        {
            return av_rescale_q(ts, codec_ctx->time_base, stream->time_base);
        }
    };

    std::unique_ptr<core::MediaSink> open_media_sink(const std::string &path, const SinkOptions &opt)
    {
        try
        {
            return std::make_unique<MediaSink>(path, opt);
        }
        catch (const std::exception &e)
        {
            return nullptr;
        }
    }
}
