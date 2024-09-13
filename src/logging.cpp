#include "logging.h"

#include "ui/helpers.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_set>

class TagFilter : public quill::Filter
{
public:
    TagFilter(std::unordered_set<std::string> tags):
        quill::Filter("TagFilter"),
        _tags(tags)
    {
    }

    bool filter(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
            std::string_view thread_id, std::string_view thread_name,
            std::string_view logger_name, quill::LogLevel log_level,
            std::string_view log_message) noexcept override
    {
        return _tags.find(std::string{logger_name}) != _tags.end();
    }

private:
    std::unordered_set<std::string> _tags;
};

static auto get_console_colors()
{
    quill::ConsoleColours colors;
    colors.set_default_colours();
    colors.set_colour(quill::LogLevel::TraceL1, quill::ConsoleColours::dark);
    colors.set_colour(quill::LogLevel::TraceL2, quill::ConsoleColours::dark);
    colors.set_colour(quill::LogLevel::TraceL3, quill::ConsoleColours::dark);

    return colors;
}

static auto get_min_loglevel()
{
    if (const char *val = std::getenv("VED_LOGLEVEL"))
    {
        return quill::loglevel_from_string(val);
    }

    return quill::LogLevel::TraceL2;
}

static std::unique_ptr<TagFilter> get_tag_filter()
{
    std::unordered_set<std::string> tags;

    if (const char *val = std::getenv("VED_LOGTAGS"))
    {
        tags.insert({val});

        return std::make_unique<TagFilter>(tags);
    }

    return nullptr;
}

static auto min_loglevel = get_min_loglevel();
static auto tag_filter = get_tag_filter();

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
        sink->set_log_level_filter(quill::LogLevel::TraceL3);

        if (tag_filter != nullptr)
            sink->add_filter(std::move(tag_filter));

        auto logger = quill::Frontend::create_or_get_logger(name, std::move(sink),
                "%(time) %(short_source_location:<28) %(log_level:<9) %(logger:<12) %(message)",
                "%s.%Qns", quill::Timezone::LocalTime, quill::ClockSourceType::User, &ui_clock);
        logger->set_log_level(min_loglevel);

        return logger;
    }

    void init()
    {
        quill::BackendOptions backend_options{};
        quill::Backend::start(backend_options);
    }
}
