#pragma once

#include <cstdint>
#include <type_traits>

namespace utility {

struct BootMailbox {
    static constexpr uint32_t kMailboxMagic = 0x524D4353;
    static constexpr uint32_t kMailboxRequestEnterDfu = 0x44465530;

    volatile uint32_t magic;
    volatile uint32_t request;

    void clear() {
        magic = 0;
        request = 0;
    }

    void request_enter_dfu() {
        magic = kMailboxMagic;
        request = kMailboxRequestEnterDfu;
    }
};

inline BootMailbox boot_mailbox __attribute__((section(".boot_mailbox"), aligned(4), used));

static_assert(std::is_standard_layout_v<BootMailbox> && sizeof(BootMailbox) <= 64);

} // namespace utility
