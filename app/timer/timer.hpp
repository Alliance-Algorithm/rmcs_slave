#pragma once

#include <main.h>
#include <tim.h>

#include "utility/immovable.hpp"
#include "utility/lazy.hpp"

namespace timer {

template <TIM_HandleTypeDef* hal_timer_handle>
class Timer : utility::Immovable {
public:
    using Lazy = utility::Lazy<Timer>;

    Timer() { HAL_TIM_Base_Start(hal_timer_handle); };

    auto get_tick() { return __HAL_TIM_GET_COUNTER(hal_timer_handle); }
};

inline constinit Timer<&htim2>::Lazy timer2;

} // namespace timer
