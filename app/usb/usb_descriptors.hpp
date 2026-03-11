#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <tuple>

#include <class/dfu/dfu.h>
#include <common/tusb_types.h>
#include <device/usbd.h>
#include <main.h>
#include <tusb_config.h>

#include "utility/lazy.hpp"

namespace usb {

class UsbDescriptors {
public:
    UsbDescriptors() { update_product_id(); }

    uint8_t const* get_device_descriptor() const {
        return reinterpret_cast<uint8_t const*>(&device_descriptor_);
    }

    static uint8_t const* get_configuration_descriptor(uint8_t index) {
        (void)index;
        return kConfigurationDescriptorFs;
    }

    uint16_t const* get_string_descriptor(uint8_t index, uint16_t langid) {
        (void)langid;
        uint8_t str_size = 0;

        if (index == 0) {
            std::memcpy(&descriptor_string_buffer_[1], kLanguageId.data(), kLanguageId.size());
            str_size = 1;
        } else {
            std::string_view str;
            switch (index) {
            case 1: str = kManufacturerString; break;
            case 2: str = kProductString; break;
            case 3: str = kDfuRuntimeString; break;
            default: return nullptr;
            }

            constexpr auto max_size = std::min<size_t>(
                std::tuple_size_v<decltype(descriptor_string_buffer_)> - 1,
                (std::numeric_limits<uint8_t>::max() - 2) / 2);

            str_size = static_cast<uint8_t>(std::min<size_t>(str.size(), max_size));
            for (uint8_t i = 0; i < str_size; ++i)
                descriptor_string_buffer_[i + 1] = static_cast<uint16_t>(str[i]);
        }

        descriptor_string_buffer_[0] =
            (TUSB_DESC_STRING << 8) | static_cast<uint16_t>((2 * str_size) + 2);
        return descriptor_string_buffer_.data();
    }

private:
    void update_product_id() {
        // Keep the legacy PID generation so host-side bindings stay stable across the port.
        auto pid = generate_legacy_product_id();
        device_descriptor_.idProduct = pid;
    }

    static uint16_t generate_legacy_product_id() {
        auto pid = static_cast<uint16_t>(0xFFFF);
        auto* data = reinterpret_cast<volatile uint8_t const*>(UID_BASE);

        for (size_t len = 12; len != 0; --len) {
            pid ^= *data++;
            for (int i = 0; i < 8; ++i) {
                if ((pid & 1U) != 0)
                    pid = static_cast<uint16_t>((pid >> 1) ^ 0x8408U);
                else
                    pid = static_cast<uint16_t>(pid >> 1);
            }
        }

        return pid;
    }

    static constexpr tusb_desc_device_t kDeviceDescriptorTemplate = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = TUSB_CLASS_VENDOR_SPECIFIC,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = 0xA11C,
        .idProduct = 0xD401,
        .bcdDevice = 0x0300,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x00,
        .bNumConfigurations = 0x01,
    };

    enum InterfaceNumber : uint8_t {
        kItfNumVendor = 0,
        kItfNumDfuRuntime,
        kItfNumTotal,
    };

    static constexpr size_t kConfigTotalLen = TUD_CONFIG_DESC_LEN
                                            + CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN
                                            + CFG_TUD_DFU_RUNTIME * TUD_DFU_RT_DESC_LEN;

    static constexpr uint8_t kEpnumVendorDataOut = 0x01;
    static constexpr uint8_t kEpnumVendorDataIn = 0x81;

    static constexpr uint8_t kConfigurationDescriptorFs[] = {
        TUD_CONFIG_DESCRIPTOR(
            1, kItfNumTotal, 0, kConfigTotalLen, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        TUD_VENDOR_DESCRIPTOR(kItfNumVendor, 0, kEpnumVendorDataOut, kEpnumVendorDataIn, 64),
        TUD_DFU_RT_DESCRIPTOR(
            kItfNumDfuRuntime, 3, DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_WILL_DETACH, 1000, 1024),
    };

    static_assert(sizeof(kConfigurationDescriptorFs) == kConfigTotalLen);

    static constexpr std::array<uint8_t, 2> kLanguageId = {0x09, 0x04};
    static constexpr std::string_view kManufacturerString = "Alliance RoboMaster Team.";
    static constexpr std::string_view kProductString = "RMCS Slave v" APP_VERSION;
    static constexpr std::string_view kDfuRuntimeString = "DFU Runtime";

    tusb_desc_device_t device_descriptor_ = kDeviceDescriptorTemplate;
    std::array<uint16_t, 128> descriptor_string_buffer_{};
};

inline constinit utility::Lazy<UsbDescriptors> usb_descriptors;

} // namespace usb
