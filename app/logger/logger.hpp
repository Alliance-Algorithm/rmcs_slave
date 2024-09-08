#pragma once

#include <bsp/SEGGER/Config/SEGGER_RTT_Conf.h>
#include <bsp/SEGGER/RTT/SEGGER_RTT.h>

#include "utility/lazy.hpp"

namespace logger {

class Logger {
public:
    using Lazy = utility::Lazy<Logger>;

    Logger() { SEGGER_RTT_Init(); }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    int printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int n = SEGGER_RTT_vprintf(0, fmt, &args);
        va_end(args);
        return n;
    }

private:
};

inline constinit Logger::Lazy logger;

}; // namespace logger