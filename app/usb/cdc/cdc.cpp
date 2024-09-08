#include "cdc.hpp"

#include "app/can/can.hpp"
#include "app/uart/uart.hpp"
#include "app/usb/field.hpp"

namespace usb {

inline int8_t hal_cdc_init_callback() {
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, reinterpret_cast<uint8_t*>(Cdc::receive_buffer_));
    return USBD_OK;
}

inline int8_t hal_cdc_deinit_callback() {
    return USBD_OK;
}

inline int8_t hal_cdc_control_callback(uint8_t command, uint8_t* buffer, uint16_t length) {
    return USBD_OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) because bullshit HAL api.
inline int8_t hal_cdc_receive_callback(uint8_t* buffer, uint32_t* length) {
    auto iterator = reinterpret_cast<std::byte*>(buffer);
    assert(iterator == Cdc::receive_buffer_);

    auto sentinel = iterator + *length;
    assert(*iterator == std::byte{0x81});
    iterator++;

    while (iterator < sentinel) {
        struct __attribute__((packed)) Header {
            field::CommandId field_id : 4;
        };
        auto field_id = std::launder(reinterpret_cast<Header*>(iterator))->field_id;

        if (field_id == field::CommandId::CAN1_) {
            if (auto can = can::can1.try_get())
                can->read_buffer_write_device(iterator);
        } else if (field_id == field::CommandId::CAN2_) {
            if (auto can = can::can2.try_get())
                can->read_buffer_write_device(iterator);
        } else if (field_id == field::CommandId::UART1_) {
            if (auto uart = uart::uart1.try_get())
                uart->read_buffer_write_device(iterator);
        } else if (field_id == field::CommandId::UART2_) {
            if (auto uart = uart::uart2.try_get())
                uart->read_buffer_write_device(iterator);
        } else if (field_id == field::CommandId::UART3_) {
            if (auto uart = uart::uart_dbus.try_get())
                uart->read_buffer_write_device(iterator);
        } else
            break;
    }
    assert(iterator == sentinel); // TODO

    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, buffer);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return USBD_OK;
}

inline int8_t
    hal_cdc_transmit_complete_callback(uint8_t* buffer, uint32_t* length, uint8_t endpoint_num) {
    return USBD_OK;
}

extern "C" {
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    hal_cdc_init_callback, hal_cdc_deinit_callback, hal_cdc_control_callback,
    hal_cdc_receive_callback, hal_cdc_transmit_complete_callback};
}

} // namespace usb
