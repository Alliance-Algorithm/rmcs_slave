#pragma once

#include "app/usb/field.hpp"
#include "app/usb/interrupt_safe_buffer.hpp"
#include "stm32f4xx_hal.h"
#include "utility/lazy.hpp"
#include <cstdint>

namespace gpio {

class TimePointSyncer {
public:
    using Lazy = utility::Lazy<TimePointSyncer>;

private:
    friend void ::HAL_GPIO_EXTI_Callback(uint16_t gpio_pin);
    void read_device_write_buffer(usb::InterruptSafeBuffer& buffer_wrapper) {
        std::byte* buffer = buffer_wrapper.allocate(sizeof(FieldHeader));
        if (buffer) {
            auto& header      = *new (buffer) FieldHeader{};
            header.field_id   = static_cast<uint8_t>(usb::field::UplinkId::TIME_POINT);
            header.time_point = HAL_GetTick();
        }
    }

    struct __attribute__((packed)) FieldHeader {
        uint8_t field_id : 4;
        uint32_t time_point;
    };
};

inline constinit TimePointSyncer::Lazy time_point_syncer;
} // namespace gpio