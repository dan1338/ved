#include "logging.h"

static auto get_console_colors()
{
    quill::ConsoleColours colors;
    colors.set_default_colours();
    colors.set_colour(quill::LogLevel::TraceL1, quill::ConsoleColours::dark);
    colors.set_colour(quill::LogLevel::TraceL2, quill::ConsoleColours::dark);
    colors.set_colour(quill::LogLevel::TraceL3, quill::ConsoleColours::dark);

    return colors;
}

namespace logging
{
    class UIClock : public quill::UserClockSource
    {
    public:
        uint64_t now() const  override
        {
            return ui::now().count();
        }
    };

    static UIClock ui_clock;

    quill::Frontend::logger_t *get_logger(const char *name)
    {
        const auto colors = get_console_colors();

        auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console", colors);
        sink->set_log_level_filter(quill::LogLevel::TraceL2);

        auto logger = quill::Frontend::create_or_get_logger(name, std::move(sink),
                "%(time) %(short_source_location:<28) %(log_level:<9) %(logger:<12) %(message)",
                "%s.%Qns", quill::Timezone::LocalTime, quill::ClockSourceType::User, &ui_clock);
        logger->set_log_level(quill::LogLevel::TraceL2);

        return logger;
    }

    void init()
    {
        quill::BackendOptions backend_options{};
        quill::Backend::start(backend_options);
    }
}
