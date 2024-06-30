#pragma once

#include <cstdint>
#include <chrono>

using namespace std::chrono_literals;

namespace core
{
    template<int64_t N, int64_t D>
    using duration = std::chrono::duration<int64_t, std::ratio<N, D>>;

    using seconds = std::chrono::seconds;

    using milliseconds = std::chrono::milliseconds;

    using microseconds = std::chrono::microseconds;

    using timestamp = duration<1, 1000000000>;

    template<typename T, typename K>
    T time_cast(K time)
    {
        return std::chrono::duration_cast<T>(time);
    }

    template<int64_t N, int64_t D>
    auto custom_duration(int64_t t)
    {
        return std::chrono::duration<int64_t, std::ratio<N, D>>(t);
    }
}

