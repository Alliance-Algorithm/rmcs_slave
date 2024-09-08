#include "uart.hpp"

#include "app/usb/cdc.hpp"

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* hal_uart_handle, uint16_t size) {
    if (__HAL_UART_GET_FLAG(hal_uart_handle, UART_FLAG_IDLE))
        return;

    uart::Uart::Lazy* uart_lazy;
    usb::field::StatusId field_id;

    if (hal_uart_handle == &huart1) {
        uart_lazy = &uart::uart2;
        field_id  = usb::field::StatusId::UART2_;
    } else if (hal_uart_handle == &huart3) {
        uart_lazy = &uart::uart_dbus;
        field_id  = usb::field::StatusId::UART3_;
    } else if (hal_uart_handle == &huart6) {
        uart_lazy = &uart::uart1;
        field_id  = usb::field::StatusId::UART1_;
    } else {
        return;
    }

    if (auto uart = uart_lazy->try_get()) {
        if (auto cdc = usb::cdc.try_get()) {
            uart->read_device_write_buffer(cdc->get_transmit_buffer(), field_id, size);
        }
    }
}
