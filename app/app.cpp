#include "app/app.hpp"

#include <device/usbd.h>
#include <main.h>

#include "app/can/can.hpp"
#include "app/spi/bmi088/accel.hpp"
#include "app/spi/bmi088/gyro.hpp"
#include "app/uart/uart.hpp"
#include "app/usb/vendor.hpp"
#include "utility/boot_mailbox.hpp"

extern "C" {
void AppEntry() { app.init().main(); }
}

App::App() {
    led::led.init();
    utility::boot_mailbox.clear();
    usb::vendor.init();
    can::can1.init();
    can::can2.init();
    uart::uart1.init();
    uart::uart2.init();
    uart::uart_dbus.init();
    spi::bmi088::accelerometer.init();
    spi::bmi088::gyroscope.init();
    __enable_irq();
};

[[noreturn]] void App::main() {
    while (true) {
        tud_task();

        usb::vendor->try_transmit();
        can::can1->try_transmit();
        usb::vendor->try_transmit();
        can::can2->try_transmit();
        usb::vendor->try_transmit();
        uart::uart1->try_transmit();
        usb::vendor->try_transmit();
        uart::uart2->try_transmit();
        usb::vendor->try_transmit();
        uart::uart_dbus->try_transmit();
    }
}
