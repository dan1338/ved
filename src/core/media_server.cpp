#include "core/media_server.h"

#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>
#include <thread>
#include <algorithm>

#include "fmt/format.h"
#include "msd/channel.hpp"

namespace core
{
    class MediaServer : public IMediaServer
    {
    public:
        std::optional<MediaHandle> open(const std::string &path) override
        {
            MediaFile file{path, avformat_alloc_context()};

            int err = avformat_open_input(&file.format_ctx, path.c_str(), nullptr, nullptr);

            if (err != 0)
            {
                avformat_free_context(file.format_ctx);
                return {};
            }

            avformat_find_stream_info(file.format_ctx, nullptr);

            file.codecs.reserve(file.format_ctx->nb_streams);

            for (size_t i = 0; i < file.format_ctx->nb_streams; i++)
            {
                const auto *stream = file.format_ctx->streams[i];
                const auto *codec = avcodec_find_decoder(stream->codecpar->codec_id);
                file.codecs.emplace_back(codec);
            }

            const MediaHandle handle = next_id();
            _files.emplace(handle, std::make_unique<MediaFile>(file));

            return handle;
        }

        void close(MediaHandle media_handle) override
        {
            auto it = _files.find(media_handle);

            if (it != _files.end())
            {
                avformat_free_context(it->second->format_ctx);
                _files.erase(it);
            }
        }

        std::optional<StreamHandle> stream_of_type(MediaHandle media_handle, AVMediaType type) override
        {
            auto &file = _files.at(media_handle);

            for (size_t i = 0; i < file->format_ctx->nb_streams; i++) 
            {
                const auto *stream = file->format_ctx->streams[i];

                if (stream->codecpar->codec_type == type)
                    return i;
            }

            return {};
        }

        RequestHandle begin_request(MediaHandle media_handle, core::timestamp position, const std::vector<StreamHandle> &streams) override
        {
            auto &file = _files.at(media_handle);

            MediaRequest request{*file, position};

            size_t num_streams = request.file.format_ctx->nb_streams;
            request.streams.reserve(num_streams);

            for (size_t i = 0; i < num_streams; i++)
            {
                auto &stream = request.streams.emplace_back();

                if (streams.empty() || std::find(streams.begin(), streams.end(), (int)i) != streams.end())
                {
                    auto *codec = request.file.codecs[i];
                    auto *codec_ctx = avcodec_alloc_context3(codec);

                    avcodec_parameters_to_context(codec_ctx, request.file.format_ctx->streams[i]->codecpar);
                    avcodec_open2(codec_ctx, codec, nullptr);
                    stream.codec_ctx = codec_ctx;
                }
                else
                {
                    stream.codec_ctx = nullptr;
                }
            }

            RequestHandle handle = next_id();
            _requests.emplace(handle, std::make_unique<MediaRequest>(request));

            for (size_t i = 0; i < num_streams; i++)
            {
                if (streams.empty() || std::find(streams.begin(), streams.end(), (int)i) != streams.end())
                {
                    _pending_requests << FrameRequest{handle, (int)i, _requests.at(handle).get()};
                }
            }

            return handle;
        }

        std::optional<MediaFrame> next_frame(RequestHandle request_handle, StreamHandle stream_handle) override
        {
            auto &request = _requests.at(request_handle);
            auto &frame_queue = request->streams[stream_handle].frame_queue;

            if (!frame_queue.empty())
            {
                auto *frame = frame_queue.back();
                frame_queue.pop();

                _pending_requests << FrameRequest{request_handle, stream_handle, _requests.at(request_handle).get()};

                return {{core::duration<1, AV_TIME_BASE>(frame->pts), frame}};
            }

            while (!_pending_responses.empty())
            {
                FrameResponse response;
                _pending_responses >> response;

                const auto &[handle, stream_index, frame] = response;

                if (response.is_eof())
                {
                    request->eof = true;
                    continue;
                }

                if (stream_index == stream_handle)
                {
                    _pending_requests << FrameRequest{request_handle, stream_handle, _requests.at(request_handle).get()};

                    return {{core::duration<1, AV_TIME_BASE>(frame->pts), frame}};
                }

                auto &request = _requests.at(handle);
                request->streams[stream_index].frame_queue.push(frame);
            }

            return {};
        }

        bool has_request_ended(RequestHandle request_handle) override
        {
            return _requests.at(request_handle)->eof;
        }

        void end_request(RequestHandle request_handle) override
        {
            auto &request = _requests.at(request_handle);

            for (auto &stream : request->streams)
            {
                auto &frame_queue = stream.frame_queue;

                while (!frame_queue.empty())
                {
                    auto *frame = frame_queue.back();
                    av_frame_free(&frame);
                    frame_queue.pop();
                }

                if (stream.codec_ctx)
                {
                    avcodec_free_context(&stream.codec_ctx);
                }
            }

            _requests.erase(request_handle);
        }

    private:
        size_t _id_counter{0};

        int next_id()
        {
            return _id_counter++;
        }

        struct MediaFile
        {
            std::string path;
            AVFormatContext *format_ctx;
            std::vector<const AVCodec*> codecs;

            // Used by decoder thread to keep track of last request handled
            // and by extension the file head position
            std::optional<RequestHandle> last_request;
        };

        std::unordered_map<MediaHandle, std::unique_ptr<MediaFile>> _files;

        struct MediaRequest
        {
            MediaFile &file;
            core::timestamp start_position;
            bool eof{false};

            // Used by decoder thread to keep track of last file position
            int64_t last_byte_position{0};

            struct Stream
            {
                AVCodecContext *codec_ctx{nullptr};
                std::queue<AVFrame*> frame_queue;

                // Used by decoder thread to skip producing frames already pending reception
                size_t unprompted_frames{0};
            };

            std::vector<Stream> streams;
        };

        std::unordered_map<RequestHandle, std::unique_ptr<MediaRequest>> _requests;

        struct FrameRequest
        {
            RequestHandle request_handle;
            int stream_index;
            MediaRequest *request;
        };

        msd::channel<FrameRequest> _pending_requests;

        struct FrameResponse
        {
            RequestHandle request_handle;
            int target_stream;
            AVFrame *frame;

            bool is_eof() const { return frame == nullptr; }
        };

        msd::channel<FrameResponse> _pending_responses;

        // Thread responsible for producing frames
        // All file accesses and codec operations are done from here
        //
        // Operations can be issued by sending FrameRequests into the input channel
        // Resulting frames can be received as FrameResponses from the output channel
        //
        // Submitting a request means a frame of desired type will be produced, but it's
        // also likely to be preceeded by frames of other types if other streams are enabled.
        // For this reason each stream has its own frame_queue, into which frames are enqueued
        // if a frame has been produced but current caller of "next_frame" wants a different one
        //
        std::thread worker_thread{[this](){
            for (const auto &[handle, target_stream, request] : _pending_requests)
            {
                int err;

                // Some other request has already submitted that frame before us
                if (auto &unprompted_frames = request->streams[target_stream].unprompted_frames)
                {
                    fmt::println("Asked for :{} but {} frames were already sent unprompted", target_stream, unprompted_frames);
                    --unprompted_frames;
                    continue;
                }

                // Check if we can use the stream as is, or if we need to seek
                auto &last_request = request->file.last_request;
                fmt::println("R{} (last_req {})", handle, *last_request);

                if (!last_request.has_value() || *last_request != handle)
                {
                    if (!request->last_byte_position)
                    {
                        // First seek is to start_position
                        const auto seek_ts = core::time_cast<core::duration<1, AV_TIME_BASE>>(request->start_position);
                        err = av_seek_frame(request->file.format_ctx, -1, seek_ts.count(), 0);
                        fmt::println("R{}:{} seek to {}s", handle, target_stream, seek_ts/1.0s);
                    }
                    else
                    {
                        // Subsequent seeks restore saved byte position
                        err = av_seek_frame(request->file.format_ctx, -1, request->last_byte_position, AVSEEK_FLAG_BYTE);
                        fmt::println("R{}:{} seek to {}bytes", handle, target_stream, request->last_byte_position);
                    }

                    if (err < 0)
                    {
                        throw fmt::system_error(err, "av_seek_frame");
                    }
                }

                AVFrame *frame{nullptr};
                AVPacket *packet = av_packet_alloc();

                // Read packets from file, decode them, send them over to be received
                // by the subsequent "next_frame" call.
                // When a frame is of desired type, the request has been fulfilled
                // All other frames that have been sent out before that are accounted for
                // by keeping track of how many "unprompted_frames" were issued
                do
                {
                    // We could be forced to send multiple frames before the desired one.
                    // Ownership is passed onto the receiver hence each frame to be sent
                    // has to be allocated anew
                    if (!frame)
                        frame = av_frame_alloc();

                    err = av_read_frame(request->file.format_ctx, packet);

                    // Ran past the end of the file
                    if (err == AVERROR_EOF)
                    {
                        _pending_responses << FrameResponse{handle, -1, nullptr};
                        break;
                    }

                    // Used to restore file position if we have to handle a different request
                    // before this one is closed
                    request->last_byte_position = avio_tell(request->file.format_ctx->pb);

                    int stream_index = packet->stream_index;
                    auto &stream = request->streams[stream_index];

                    // Drop packets of unwanted streams
                    if (!stream.codec_ctx)
                    {
                        av_packet_unref(packet);
                        continue;
                    }

                    err = avcodec_send_packet(stream.codec_ctx, packet);
                    av_packet_unref(packet);

                    err = avcodec_receive_frame(stream.codec_ctx, frame);

                    // For now, assume any failure to be EAGAIN
                    // video frames are usually composed of many packets
                    if (err == 0)
                    {
                        // Submit frame and unset our local pointer, for it to be reallocated in next cycle
                        _pending_responses << FrameResponse{handle, stream_index, frame};
                        frame = nullptr;

                        if (stream_index == target_stream)
                        {
                            // Request completed
                            break;
                        }
                        else
                        {
                            // Not what we asked for but ok
                            ++stream.unprompted_frames;
                        }
                    }
                }
                while (1);

                av_frame_free(&frame);
                av_packet_free(&packet);

                // Mark current request as last handled
                last_request.emplace(handle);
                fmt::println("end R{}", handle);
            }
        }};
    };

    static std::unique_ptr<MediaServer> media_server_instance;

    void start_media_server()
    {
        media_server_instance = std::make_unique<MediaServer>();
    }

    IMediaServer *get_media_server()
    {
        return media_server_instance.get();
    }
}

