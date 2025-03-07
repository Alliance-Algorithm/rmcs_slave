#pragma once

#include <cstdint>

#include <atomic>

#include <usbd_cdc.h>
#include <usbd_def.h>

#include "app/usb/interrupt_safe_buffer.hpp"
#include "utility/assert.hpp"
#include "utility/lazy.hpp"

namespace usb {

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceFS;
}

class Cdc : utility::Immovable {
public:
    using Lazy = utility::Lazy<Cdc>;

    Cdc() = default;

    InterruptSafeBuffer& get_transmit_buffer() { return transmit_buffer_; }

    bool try_transmit() {
        if (!device_ready())
            return false;

        if (connecting_.load(std::memory_order::relaxed)) {
            transmit_buffer_.clear();
            connecting_.store(false, std::memory_order::relaxed);
            std::atomic_signal_fence(std::memory_order_release);
            led::led->reset();
            return false;
        }

        auto batch = transmit_buffer_.pop_batch();
        if (!batch)
            return false;

        auto written_size = batch->written_size.load(std::memory_order::relaxed);
        batch->written_size.store(1, std::memory_order::relaxed);

        auto data = reinterpret_cast<uint8_t*>(batch->data);

        assert_always(
            USBD_CDC_SetTxBuffer(&hUsbDeviceFS, data, written_size) == USBD_OK
            && USBD_CDC_TransmitPacket(&hUsbDeviceFS) == USBD_OK);
        return true;
    }

private:
    static bool device_ready() {
        // The value of cdc_handle remains null until a USB connection occurs, and an interrupt
        // modifies it to a non-zero value. Therefore, atomic loading must be used here to prevent
        // compiler optimization.

        auto hal_cdc_handle_atomic =
            std::atomic_ref<void*>(hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId]);
        void* hal_cdc_handle = hal_cdc_handle_atomic.load(std::memory_order_relaxed);

        if (!hal_cdc_handle)
            return false;

        return static_cast<USBD_CDC_HandleTypeDef*>(hal_cdc_handle)->TxState == 0U;
    }

    void read_control_field(std::byte*& buffer) {
        enum class Command : uint8_t {
            CONNECT = 0, // Clear uplink buffer, reset alarm and configuration
        };
        struct __attribute__((packed)) FieldHeader {
            uint8_t field_id : 4;
            Command command  : 4;
        };

        auto header = std::bit_cast<FieldHeader>(*buffer++);
        if (header.command == Command::CONNECT) {
            connecting_.store(true, std::memory_order::relaxed);
        } else {
            assert(false);
            __builtin_unreachable();
        }
    }

    friend inline int8_t hal_cdc_init_callback();
    friend inline int8_t hal_cdc_deinit_callback();
    friend inline int8_t hal_cdc_control_callback(uint8_t, uint8_t*, uint16_t);
    friend inline int8_t hal_cdc_receive_callback(uint8_t*, uint32_t*);
    friend inline int8_t hal_cdc_transmit_complete_callback(uint8_t*, uint32_t*, uint8_t);

    alignas(size_t) inline static constinit std::byte receive_buffer_[64];
    InterruptSafeBuffer transmit_buffer_{};

    std::atomic<bool> connecting_;
};

inline constinit Cdc::Lazy cdc;

} // namespace usb
