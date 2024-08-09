#pragma once

#include <cassert>
#include <cstdint>

#include <atomic>

#include <usbd_cdc.h>
#include <usbd_def.h>

#include "utility/interrupt_safe_buffer.hpp"
#include "utility/lazy.hpp"

namespace device::usb {

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceFS;
}

class Cdc : utility::Immovable {
public:
    using Lazy = utility::Lazy<Cdc>;

    Cdc() {
        for (auto& buffer : transmit_buffers_) {
            std::byte* start_of_packet = buffer.allocate(1);
            assert(start_of_packet);
            *start_of_packet = std::byte{0xAE};
        }
    };

    utility::InterruptSafeBuffer<64>& get_transmit_buffer() {
        return transmit_buffers_[buffer_writing_.load(std::memory_order::relaxed)];
    }

    bool try_transmit() {
        auto writing = buffer_writing_.load(std::memory_order::relaxed);
        if (transmit_buffers_[writing].written_size() <= 1)
            return false;

        if (!device_ready())
            return false;

        transmit_buffers_[!writing].set_written_size(1);
        std::atomic_signal_fence(std::memory_order::release);
        buffer_writing_.store(!writing, std::memory_order::relaxed);
        std::atomic_signal_fence(std::memory_order::release);

        auto data = reinterpret_cast<uint8_t*>(transmit_buffers_[writing].data());

        // Note: Must read written_size again here to avoid packet loss.
        assert(
            USBD_CDC_SetTxBuffer(&hUsbDeviceFS, data, transmit_buffers_[writing].written_size())
                == USBD_OK
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

    friend inline int8_t hal_cdc_init_callback();
    friend inline int8_t hal_cdc_deinit_callback();
    friend inline int8_t hal_cdc_control_callback(uint8_t, uint8_t*, uint16_t);
    friend inline int8_t hal_cdc_receive_callback(uint8_t*, uint32_t*);
    friend inline int8_t hal_cdc_transmit_complete_callback(uint8_t*, uint32_t*, uint8_t);

    alignas(int) inline static constinit std::byte receive_buffer_[64];

    utility::InterruptSafeBuffer<64> transmit_buffers_[2];
    std::atomic<uint8_t> buffer_writing_ = 0;
};

inline constinit Cdc::Lazy cdc;

} // namespace device::usb
