#pragma once

#include "app/usb/field.hpp"

namespace spi::bmi088 {

struct __attribute__((packed)) FieldHeader {
    usb::field::UplinkId field_id : 4;
    enum class DeviceId : uint8_t {
        ACCELEROMETER = 0,
        GYROSCOPE     = 1,
    } device_id : 4;

    static constexpr FieldHeader accelerometer() {
        return FieldHeader{usb::field::UplinkId::IMU_, DeviceId::ACCELEROMETER};
    }

    static constexpr FieldHeader gyroscope() {
        return FieldHeader{usb::field::UplinkId::IMU_, DeviceId::GYROSCOPE};
    }
};

} // namespace spi::bmi088