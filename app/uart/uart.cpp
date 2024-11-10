#include "uart.hpp"

#include "app/usb/cdc.hpp"

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* hal_uart_handle, uint16_t size) {
    if (__HAL_UART_GET_FLAG(hal_uart_handle, UART_FLAG_IDLE))
        return;

    uart::Uart* uart;
    usb::field::StatusId field_id;

    if (hal_uart_handle == &huart1) {
        uart     = uart::uart2.get();
        field_id = usb::field::StatusId::UART2_;
    } else if (hal_uart_handle == &huart3) {
        uart     = uart::uart_dbus.get();
        field_id = usb::field::StatusId::UART3_;
    } else if (hal_uart_handle == &huart6) {
        uart     = uart::uart1.get();
        field_id = usb::field::StatusId::UART1_;
    } else {
        return;
    }

    uart->read_device_write_buffer(usb::cdc->get_transmit_buffer(), field_id, size);
}
