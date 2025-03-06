#pragma once

#include <cstdint>
#include <cstdio>

#include <spi.h>
#include <usbd_cdc.h>

#include "app/spi/bmi088/field.hpp"
#include "app/spi/spi.hpp"
#include "app/timer/delay.hpp"
#include "app/usb/cdc.hpp"
#include "utility/assert.hpp"

namespace spi::bmi088 {

class Accelerometer final : SpiModuleInterface {
public:
    using Lazy = utility::Lazy<Accelerometer, Spi::Lazy*>;

    enum class Range : uint8_t { _3G = 0x00, _6G = 0x01, _12G = 0x02, _24G = 0x03 };
    enum class DataRate : uint8_t {
        _12   = 0x05,
        _25   = 0x06,
        _50   = 0x07,
        _100  = 0x08,
        _200  = 0x09,
        _400  = 0x0A,
        _800  = 0x0B,
        _1600 = 0x0C
    };

    struct __attribute__((packed)) Data {
        int16_t x;
        int16_t y;
        int16_t z;
    };

    explicit Accelerometer(
        Spi::Lazy* spi, Range range = Range::_6G, DataRate data_rate = DataRate::_1600)
        : SpiModuleInterface(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin)
        , spi_(spi->init())
        , initialized_(false)
        , init_rx_buffer_(nullptr)
        , init_rx_size_(0) {

        using namespace std::chrono_literals;

        constexpr int max_try_time = 3;

        auto read_with_confirm = [this](RegisterAddress address, uint8_t value) {
            for (int i = max_try_time; i-- > 0;) {
                if (!read<SpiTransmitReceiveMode::BLOCK>(address, 1))
                    return false;
                assert_always(init_rx_buffer_ && init_rx_size_ == 3);
                if (init_rx_buffer_[2] == value)
                    return true;
                timer::delay(1ms);
            }
            return false;
        };
        auto write_with_confirm = [this](RegisterAddress address, uint8_t value) {
            for (int i = max_try_time; i-- > 0;) {
                if (!write<SpiTransmitReceiveMode::BLOCK>(address, value))
                    return false;
                timer::delay(1ms);
                if (!read<SpiTransmitReceiveMode::BLOCK>(address, 1))
                    return false;
                assert_always(init_rx_buffer_ && init_rx_size_ == 3);
                if (init_rx_buffer_[2] == value)
                    return true;
            }
            return false;
        };
        // Dummy read to switch accelerometer to SPI mode
        read<SpiTransmitReceiveMode::BLOCK>(RegisterAddress::ACC_CHIP_ID, 1);
        timer::delay(1ms);
        HAL_Delay(1);
        read<SpiTransmitReceiveMode::BLOCK>(RegisterAddress::ACC_CHIP_ID, 1);
        timer::delay(1ms);
        HAL_Delay(1);

        // Reset all registers to reset value
        write<SpiTransmitReceiveMode::BLOCK>(RegisterAddress::ACC_SOFTRESET, 0xB6);
        timer::delay(1ms);
        HAL_Delay(1);

        // "Who am I" check.E
        read_with_confirm(RegisterAddress::ACC_CHIP_ID, 0x1E);
        timer::delay(1ms);
        HAL_Delay(1);
        assert_always(read_with_confirm(RegisterAddress::ACC_CHIP_ID, 0x1E));
        // Enable INT1 as output pin, push-pull, active-low.
        assert_always(write_with_confirm(RegisterAddress::INT1_IO_CTRL, 0b00001000));
        // Map data ready interrupt to pin INT1.
        assert_always(write_with_confirm(RegisterAddress::INT_MAP_DATA, 0b00000100));

        // Set ODR (output data rate) = 1600 and OSR (over-sampling-ratio) = 1.
        assert_always(write_with_confirm(
            RegisterAddress::ACC_CONF,
            0x80 | (0x02 << 4) | (static_cast<uint8_t>(data_rate) << 0)));
        // Set Accelerometer range.
        assert_always(write_with_confirm(RegisterAddress::ACC_RANGE, static_cast<uint8_t>(range)));

        // Switch the accelerometer into active mode.
        assert_always(write_with_confirm(RegisterAddress::ACC_PWR_CONF, 0x00));
        // Turn on the accelerometer.
        assert_always(write_with_confirm(RegisterAddress::ACC_PWR_CTRL, 0x04));

        initialized_ = true;
    }

private:
    friend void ::HAL_GPIO_EXTI_Callback(uint16_t);

    void data_ready_callback() {
        read<SpiTransmitReceiveMode::BLOCK>(RegisterAddress::ACC_X_LSB, 6);
    }

protected:
    void transmit_receive_callback(uint8_t* rx_buffer, size_t size) override {
        if (initialized_) {
            assert(size == sizeof(Data) + 2);
            auto& data = *std::launder(reinterpret_cast<Data*>(rx_buffer + 2));
            read_device_write_buffer(usb::cdc->get_transmit_buffer(), data);
        } else {
            init_rx_buffer_ = rx_buffer;
            init_rx_size_   = size;
        }
    }

private:
    enum class RegisterAddress : uint8_t {
        ACC_SOFTRESET  = 0x7E,
        ACC_PWR_CTRL   = 0x7D,
        ACC_PWR_CONF   = 0x7C,
        ACC_SELF_TEST  = 0x6D,
        INT_MAP_DATA   = 0x58,
        INT2_IO_CTRL   = 0x54,
        INT1_IO_CTRL   = 0x53,
        ACC_RANGE      = 0x41,
        ACC_CONF       = 0x40,
        TEMP_LSB       = 0x23,
        TEMP_MSB       = 0x22,
        ACC_INT_STAT_1 = 0x1D,
        SENSORTIME_2   = 0x1A,
        SENSORTIME_1   = 0x19,
        SENSORTIME_0   = 0x18,
        ACC_Z_MSB      = 0x17,
        ACC_Z_LSB      = 0x16,
        ACC_Y_MSB      = 0x15,
        ACC_Y_LSB      = 0x14,
        ACC_X_MSB      = 0x13,
        ACC_X_LSB      = 0x12,
        ACC_STATUS     = 0x03,
        ACC_ERR_REG    = 0x02,
        ACC_CHIP_ID    = 0x00
    };

    template <SpiTransmitReceiveMode mode>
    bool write(RegisterAddress address, uint8_t value) {
        if (auto task = spi_.create_transmit_receive_task<mode>(this, 2)) {
            task->tx_buffer[0] = static_cast<uint8_t>(address);
            task->tx_buffer[1] = value;
            return true;
        }
        return false;
    }

    template <SpiTransmitReceiveMode mode>
    bool read(RegisterAddress address, size_t read_size) {
        if (auto task = spi_.create_transmit_receive_task<mode>(this, read_size + 2)) {
            task->tx_buffer[0] = 0x80 | static_cast<uint8_t>(address);
            task->tx_buffer[1] = 0x55;
            task->tx_buffer[2] = 0x55;
            return true;
        }
        return false;
    }

    static bool read_device_write_buffer(usb::InterruptSafeBuffer& buffer_wrapper, Data& data) {
        if (std::byte* buffer = buffer_wrapper.allocate(sizeof(FieldHeader) + sizeof(Data))) {
            *buffer = std::bit_cast<std::byte>(FieldHeader::accelerometer());
            buffer += sizeof(FieldHeader);

            new (buffer) Data{data};
            buffer += sizeof(Data);

            return true;
        }

        return false;
    }

    Spi& spi_;

    bool initialized_;

    uint8_t* init_rx_buffer_;
    size_t init_rx_size_;
};

inline constinit Accelerometer::Lazy accelerometer(&spi1);

} // namespace spi::bmi088
