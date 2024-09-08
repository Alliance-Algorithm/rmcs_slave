#include "app/app.hpp"

#include <main.h>

#include "app/can/can.hpp"
#include "app/spi/bmi088/accel.hpp"
#include "app/spi/bmi088/gyro.hpp"
#include "app/uart/uart.hpp"
#include "app/usb/cdc.hpp"

extern "C" {
void AppEntry() { app->main(); }
}

App::App() = default;

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[noreturn]] void App::main() {
    auto& cdc       = *usb::cdc;
    auto& can1      = *can::can1;
    auto& can2      = *can::can2;
    auto& uart1     = *uart::uart1;
    auto& uart2     = *uart::uart2;
    auto& uart_dbus = *uart::uart_dbus;
    auto& accel     = *spi::bmi088::accelerometer;
    auto& gyro      = *spi::bmi088::gyroscope;

    (void)accel;
    (void)gyro;

    while (true) {
        cdc.try_transmit();
        can1.try_transmit();
        cdc.try_transmit();
        can2.try_transmit();
        cdc.try_transmit();
        uart1.try_transmit();
        cdc.try_transmit();
        uart2.try_transmit();
        cdc.try_transmit();
        uart_dbus.try_transmit();
    }
}