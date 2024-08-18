#pragma once

#include <vector>
#include <functional>

namespace core
{
    template<typename ...CallbackArgs>
    struct Event
    {
        using CallbackType = std::function<void(CallbackArgs...)>;

        std::vector<CallbackType> _callbacks;

        void add_callback(const CallbackType &cb)
        {
            _callbacks.push_back(cb);
        }

        void notify(CallbackArgs... args)
        {
            for (const auto &cb : _callbacks)
            {
                cb(args...);
            }
        }
    };
}

