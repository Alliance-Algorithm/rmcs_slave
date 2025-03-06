#include "uart.hpp"

#include "app/usb/cdc.hpp"

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* hal_uart_handle, uint16_t size) {
    if (__HAL_UART_GET_FLAG(hal_uart_handle, UART_FLAG_IDLE))
        return;

    uart::Uart* uart;
    usb::field::UplinkId field_id;

    if (hal_uart_handle == &huart1) {
        uart     = uart::uart1.get();
        field_id = usb::field::UplinkId::UART2_;
    } else if (hal_uart_handle == &huart7) {
        uart     = uart::uart2.get();
        field_id = usb::field::UplinkId::UART4_;
    } else if (hal_uart_handle == &huart10) {
        uart     = uart::uart3.get();
        field_id = usb::field::UplinkId::UART1_;
    } else if (hal_uart_handle == &huart5) {
        uart     = uart::uart_dbus.get();
        field_id = usb::field::UplinkId::UART3_;
    } else {
        return;
    }

    uart->read_device_write_buffer(usb::cdc->get_transmit_buffer(), field_id, size);
}
