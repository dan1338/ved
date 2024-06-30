#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/time.h"

#include "ffmpeg/headers.h"

namespace core
{
    class IMediaServer
    {
    public:
        using MediaHandle = int;
        using StreamHandle = int;

        virtual std::optional<MediaHandle> open(const std::string &path) = 0;
        virtual void close(MediaHandle media_handle) = 0;
        virtual std::optional<StreamHandle> stream_of_type(MediaHandle media_handle, AVMediaType type) = 0;

        using RequestHandle = int;
        using MediaFrame = std::pair<core::timestamp, AVFrame*>;

        virtual RequestHandle begin_request(MediaHandle media_handle, core::timestamp position, const std::vector<StreamHandle> &streams = {}) = 0;
        virtual std::optional<MediaFrame> next_frame(RequestHandle request_handle, StreamHandle stream_handle) = 0;
        virtual bool has_request_ended(RequestHandle request_handle) = 0;
        virtual void end_request(RequestHandle request_handle) = 0;
    };

    void start_media_server();
    IMediaServer *get_media_server();
}

