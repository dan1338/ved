#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#include "fmt/base.h"
#include "fmt/format.h"

namespace logging
{
    using namespace std::chrono_literals;

    static std::chrono::high_resolution_clock clk;
    static const auto time_origin = clk.now();

    static struct TraceFile {
        FILE *fp;

        TraceFile(): fp(fopen("/dev/stdout", "w")) {}
        ~TraceFile() { fclose(fp); }
    } output_file;

    static std::string make_prefix(const std::string name)
    {
        const auto timestamp = clk.now() - time_origin;

        return fmt::format("[{:.9f}] {} : ", timestamp / 1.0s, name);
    }

    template<typename ...T>
    static void info(const std::string &format, T&&... args)
    {
        fmt::println(output_file.fp, make_prefix("INFO") + format, args...);
    }

    template<typename ...T>
    static void warn(const std::string &format, T&&... args)
    {
        fmt::println(output_file.fp, make_prefix("WARN") + format, args...);
    }

    template<typename ...T>
    static void error(const std::string &format, T&&... args)
    {
        fmt::println(output_file.fp, make_prefix("ERROR") + format, args...);
    }
}

