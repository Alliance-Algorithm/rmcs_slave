#include "app/timer/delay.hpp"

extern "C" {

// The STM32 DWT (Data Watchpoint and Trace) unit is used to rewrite the Hal_Delay function to
// ensure that it works when interrupts are disabled, while significantly improving accuracy.
void HAL_Delay(uint32_t delay) { timer::delay(std::chrono::milliseconds(delay)); }

} // extern "C"