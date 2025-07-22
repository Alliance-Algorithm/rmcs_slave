#pragma once

#include "app/usb/field.hpp"
#include "app/usb/interrupt_safe_buffer.hpp"
#include "main.h"
#include "tim.h"
#include "utility/lazy.hpp"

namespace gpio {
class BulletChecker {
public:
    using Lazy = utility::Lazy<BulletChecker>;

    BulletChecker() { HAL_TIM_Base_Start_IT(&htim2); }

private:
    friend void ::HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);

    void read_device_write_buffer(usb::InterruptSafeBuffer& buffer_wrapper) {
        const auto bullet_checker_level =
            static_cast<bool>(HAL_GPIO_ReadPin(BULLET_CHECKER_GPIO_Port, BULLET_CHECKER_Pin));

        std::byte* buffer = buffer_wrapper.allocate(sizeof(FieldHeader) + sizeof(FieldBody));
        if (buffer) {
            auto& header                = *new (buffer) FieldHeader{};
            header.field_id             = static_cast<uint8_t>(usb::field::UplinkId::GPIO_);
            header.gpio_id              = 0b10;

            auto& body     = *new (buffer + sizeof(FieldHeader)) FieldBody{};
            body.gpio_data = bullet_checker_level ? 0b10 : 0b00;
        }
    }

    struct __attribute__((packed)) FieldHeader {
        uint8_t field_id : 4;
        // 0b10 bullet checker
        // 0b01 camera capturer
        uint8_t gpio_id : 2;
    };

    struct __attribute__((packed)) FieldBody {
        uint8_t gpio_data : 2;
    };
};

inline constinit BulletChecker::Lazy bullet_checker;
} // namespace gpio