#pragma once

#include <atomic>
#include <cstddef>
#include <span>

#include "app/usb/interrupt_safe_buffer.hpp"
#include "utility/immovable.hpp"
#include "utility/lazy.hpp"

namespace usb {

class Vendor : utility::Immovable {
public:
    using Lazy = utility::Lazy<Vendor>;

    static constexpr size_t kMaxPacketSize = 64;

    Vendor();

    InterruptSafeBuffer& get_transmit_buffer();

    void handle_downlink(std::span<const std::byte> buffer);

    bool try_transmit();

private:
    void read_control_field(std::byte*& buffer);

    InterruptSafeBuffer transmit_buffer_{};
    std::atomic<bool> connecting_{false};
};

inline constinit Vendor::Lazy vendor;

} // namespace usb
