#pragma once

#include "device/usb/field.hpp"

namespace device::spi::bmi088 {

struct __attribute__((packed)) FieldHeader {
    usb::field::StatusId field_id : 4;
    enum class DeviceId : uint8_t {
        ACCELEROMETER = 0,
        GYROSCOPE     = 1,
    } device_id : 4;

    static constexpr FieldHeader accelerometer() {
        return FieldHeader{usb::field::StatusId::IMU_, DeviceId::ACCELEROMETER};
    }

    static constexpr FieldHeader gyroscope() {
        return FieldHeader{usb::field::StatusId::IMU_, DeviceId::GYROSCOPE};
    }
};

} // namespace device::spi::bmi088