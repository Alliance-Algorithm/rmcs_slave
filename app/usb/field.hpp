#pragma once

#include <cstdint>

namespace usb::field {

// All id enumeration items have a FUCKING underscore.
// Because the FUCKING HAL library defines these SHORT words as FUCKING GLOBAL macros.

enum class StatusId : uint8_t {
    RESERVED_ = 0,

    GPIO_ = 1,

    CAN1_ = 2,
    CAN2_ = 3,
    CAN3_ = 4,

    UART1_ = 5,
    UART2_ = 6,
    UART3_ = 7,
    UART4_ = 8,
    UART5_ = 9,
    UART6_ = 10,

    IMU_ = 11,
};

enum class CommandId : uint8_t {
    RESERVED_ = 0,

    GPIO_ = 1,

    CAN1_ = 2,
    CAN2_ = 3,
    CAN3_ = 4,

    UART1_ = 5,
    UART2_ = 6,
    UART3_ = 7,
    UART4_ = 8,
    UART5_ = 9,
    UART6_ = 10,

    LED_    = 11,
    BUZZER_ = 12,
};

} // namespace usb::field