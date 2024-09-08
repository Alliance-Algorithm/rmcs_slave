#include <cstdint>

#include <main.h>
#include <stm32f4xx_hal_gpio.h>

#include "app/spi/bmi088/accel.hpp"
#include "app/spi/bmi088/gyro.hpp"

extern "C" {

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin) {
    if (gpio_pin == INT1_ACC_Pin) {
        if (auto accel = spi::bmi088::accelerometer.try_get())
            accel->data_ready_callback();
    } else if (gpio_pin == INT1_GYRO_Pin) {
        if (auto gyro = spi::bmi088::gyroscope.try_get())
            gyro->data_ready_callback();
    }
}

}; // extern "C"