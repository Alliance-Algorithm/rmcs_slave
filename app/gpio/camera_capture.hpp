#pragma once

#include "app/usb/field.hpp"
#include "app/usb/interrupt_safe_buffer.hpp"
#include <cstdint>

namespace gpio::camera_capturer {
struct __attribute__((packed)) FieldHeader {
    uint8_t field_id : 4;
    // 0b01 bullet checker
    // 0b10 camera capturer
    uint8_t gpio_id : 2;
};

struct __attribute__((packed)) FieldBody {
    uint8_t gpio_data : 2;
};
static inline void camera_capturer_callback(usb::InterruptSafeBuffer& buffer_wrapper) {
    std::byte* buffer = buffer_wrapper.allocate(sizeof(FieldHeader) + sizeof(FieldBody));
    if (buffer) {
        ::new (buffer)
            FieldHeader{static_cast<uint8_t>(usb::field::UplinkId::GPIO_), 0b01};
        ::new (buffer + sizeof(FieldHeader))
            FieldBody{0b01};
    }
}

} // namespace gpio::camera_capturer