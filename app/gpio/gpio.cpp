#include <cstdint>

#include <main.h>
#include <stm32f4xx_hal_gpio.h>

#include "app/led/led.hpp"
#include "app/spi/bmi088/accel.hpp"
#include "app/spi/bmi088/gyro.hpp"
#include "app/usb/cdc.hpp"
#include "bullet_checker.hpp"

extern "C" {

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin) {
    if (gpio_pin == INT1_ACC_Pin) {
        spi::bmi088::accelerometer->data_ready_callback();
    } else if (gpio_pin == INT1_GYRO_Pin) {
        spi::bmi088::gyroscope->data_ready_callback();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim == &htim2) {
        gpio::bullet_checker.get()->read_device_write_buffer(usb::cdc->get_transmit_buffer());
    }
}

}; // extern "C"