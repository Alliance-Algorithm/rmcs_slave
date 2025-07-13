#include "app/timer/delay.hpp"
#include "app/led/led.hpp"

#include <stm32h7xx_hal.h>

extern "C" {

// The STM32 DWT (Data Watchpoint and Trace) unit is used to rewrite the Hal_Delay function to
// ensure that it works when interrupts are disabled, while significantly improving accuracy.
void HAL_Delay(uint32_t delay) { timer::delay(std::chrono::milliseconds(delay)); }

// Hack this useless function to perform regular low-priority tasks, eliminating the need for a
// dedicated timer peripheral.
void HAL_IncTick() {
    uint32_t tick = uwTick + 1;
    uwTick        = tick;
    led::led->update(tick);
}

} // extern "C"