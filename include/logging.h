#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#include "quill/Frontend.h"
#include "quill/Backend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

namespace logging
{
    static auto get_console_colors()
    {
        quill::ConsoleColours colors;
        colors.set_default_colours();
        colors.set_colour(quill::LogLevel::TraceL1, quill::ConsoleColours::dark);
        colors.set_colour(quill::LogLevel::TraceL2, quill::ConsoleColours::dark);
        colors.set_colour(quill::LogLevel::TraceL3, quill::ConsoleColours::dark);

        return colors;
    }

    static auto get_logger(const char *name)
    {
        const auto colors = get_console_colors();

        auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console", colors);
        sink->set_log_level_filter(quill::LogLevel::TraceL3);

        auto logger = quill::Frontend::create_or_get_logger(name, std::move(sink));
        logger->set_log_level(quill::LogLevel::TraceL3);

        return logger;
    }

    static void init()
    {
        quill::BackendOptions backend_options{};
        quill::Backend::start(backend_options);
    }
}

