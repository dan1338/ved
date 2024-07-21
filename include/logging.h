#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#include "quill/Frontend.h"
#include "quill/Backend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/ConsoleSink.h"

#include "ui/helpers.h"

namespace logging
{
    void init();
    quill::Frontend::logger_t *get_logger(const char *name);
}

