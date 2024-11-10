#pragma once

#include <cassert>
#include <cstdint>

#include <chrono>
#include <limits>
#include <ratio>

#include <stm32f407xx.h>

namespace timer {

constexpr uint32_t system_frequency = 168'000'000;
using SysFreqDuration = std::chrono::duration<uint32_t, std::ratio<1, system_frequency>>;

inline void delay_basic(SysFreqDuration delay) {
    if (!delay.count()) [[unlikely]]
        return;

    uint32_t start = DWT->CYCCNT;
    uint32_t end   = start + delay.count();

    if (end < start) { // Overflow
        while (DWT->CYCCNT >= start)
            __NOP();
    }

    while (DWT->CYCCNT < end)
        __NOP();
}

template <std::integral Rep, typename Period>
inline void delay(std::chrono::duration<Rep, Period> delay) {
    if constexpr (std::is_signed_v<Rep>) {
        if (delay.count() < 0) [[unlikely]]
            return;
    }

    if constexpr (sizeof(Rep) > sizeof(uint32_t)) {
        assert(delay.count() <= std::numeric_limits<uint32_t>::max());
    }

    using DurationT    = std::chrono::duration<uint32_t, Period>;
    auto casted_delay  = DurationT{delay.count()};
    constexpr auto max = std::chrono::floor<DurationT>(SysFreqDuration::max());
    static_assert(max.count() > 0, "The unit is too large, please choose a smaller unit");

    while (casted_delay > max) {
        casted_delay -= max;
        delay_basic(SysFreqDuration::max());
    }
    delay_basic(std::chrono::round<SysFreqDuration>(casted_delay));
}

} // namespace timer