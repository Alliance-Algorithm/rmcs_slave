#include "app/usb/vendor.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

#include <class/vendor/vendor_device.h>
#include <main.h>
#include <tusb.h>

#include "app/can/can.hpp"
#include "app/led/led.hpp"
#include "app/uart/uart.hpp"
#include "app/usb/field.hpp"
#include "app/usb/usb_descriptors.hpp"
#include "utility/assert.hpp"
#include "utility/boot_mailbox.hpp"

namespace usb {

Vendor::Vendor() {
    usb_descriptors.init();
    assert_always(tusb_rhport_init(0, nullptr));
}

InterruptSafeBuffer& Vendor::get_transmit_buffer() { return transmit_buffer_; }

void Vendor::handle_downlink(std::span<const std::byte> buffer) {
    if (buffer.empty())
        return;

    auto* iterator = const_cast<std::byte*>(buffer.data());
    auto* sentinel = iterator + buffer.size();

    assert_always(*iterator == std::byte{0x81});
    ++iterator;

    while (iterator < sentinel) {
        struct __attribute__((packed)) Header {
            field::DownlinkId field_id : 4;
        };
        static_assert(sizeof(Header) == 1);

        const auto field_id = std::bit_cast<Header>(*iterator).field_id;

        if (field_id == field::DownlinkId::CONTROL_) {
            read_control_field(iterator);
        } else if (field_id == field::DownlinkId::CAN1_) {
            can::can1->read_buffer_write_device(iterator);
        } else if (field_id == field::DownlinkId::CAN2_) {
            can::can2->read_buffer_write_device(iterator);
        } else if (field_id == field::DownlinkId::UART1_) {
            uart::uart1->read_buffer_write_device(iterator);
        } else if (field_id == field::DownlinkId::UART2_) {
            uart::uart2->read_buffer_write_device(iterator);
        } else if (field_id == field::DownlinkId::UART3_) {
            uart::uart_dbus->read_buffer_write_device(iterator);
        } else {
            break;
        }
    }

    assert_always(iterator == sentinel);
}

bool Vendor::try_transmit() {
    if (!tud_ready())
        return false;

    if (connecting_.load(std::memory_order::relaxed)) {
        transmit_buffer_.clear();
        connecting_.store(false, std::memory_order::relaxed);
        std::atomic_signal_fence(std::memory_order_release);
        led::led->reset();
        return false;
    }

    if (!tud_vendor_n_write_available(0))
        return false;

    auto* batch = transmit_buffer_.pop_batch();
    if (!batch)
        return false;

    const auto written_size = batch->written_size.load(std::memory_order::relaxed);
    batch->written_size.store(1, std::memory_order::relaxed);

    const auto* data = reinterpret_cast<const uint8_t*>(batch->data);
    assert_always(tud_vendor_n_write(0, data, written_size) == written_size);
    return true;
}

void Vendor::read_control_field(std::byte*& buffer) {
    enum class Command : uint8_t {
        CONNECT = 0,
    };

    struct __attribute__((packed)) FieldHeader {
        uint8_t field_id : 4;
        Command command : 4;
    };
    static_assert(sizeof(FieldHeader) == 1);

    const auto header = std::bit_cast<FieldHeader>(*buffer++);
    if (header.command == Command::CONNECT) {
        connecting_.store(true, std::memory_order::relaxed);
        return;
    }

    assert(false);
    __builtin_unreachable();
}

} // namespace usb

extern "C" {

void tud_vendor_rx_cb(uint8_t itf, const uint8_t* buffer, uint32_t size) {
    if (itf != 0) [[unlikely]]
        return;

    usb::vendor->handle_downlink(
        {reinterpret_cast<const std::byte*>(buffer), static_cast<size_t>(size)});
}

void tud_dfu_runtime_reboot_to_dfu_cb() {
    utility::boot_mailbox.request_enter_dfu();
    __DSB();
    __ISB();
    NVIC_SystemReset();
}

void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }

void tud_resume_cb() {}

void tud_mount_cb() {}

void tud_umount_cb() {}

} // extern "C"
