#pragma once

#include <cstddef>

#include <atomic>
#include <bit>

#include "app/led/led.hpp"
#include "utility/assert.hpp"
#include "utility/immovable.hpp"

namespace usb {

class InterruptSafeBuffer final : utility::Immovable {
public:
    friend class Cdc;

    static constexpr size_t batch_size  = 64;
    static constexpr size_t batch_count = 8;
    static_assert(std::has_single_bit(batch_count), "Batch count must be a power of 2");

    constexpr InterruptSafeBuffer() {
        for (auto& batch : batches_) {
            std::byte* start_of_packet = batch.allocate(1);
            assert_always(start_of_packet);
            *start_of_packet = std::byte{0xAE};
        }
    };

    std::byte* allocate(size_t size) {
        assert(size <= batch_size);

        auto out = out_.load(std::memory_order::relaxed);

        while (true) {
            auto in = in_.load(std::memory_order::relaxed);

            auto readable = in - out;
            if (readable) {
                if (auto result = batches_[(in - 1) & mask].allocate(size))
                    return result;
            }

            auto writeable = batch_count - readable - 1;
            if (!writeable) {
                led::led->uplink_buffer_full();
                return nullptr;
            }

            in_.compare_exchange_weak(in, in + 1, std::memory_order::relaxed);
        }
    }

private:
    static constexpr size_t mask = batch_count - 1;

    struct Batch {
        std::byte* allocate(size_t size) {
            size_t written_size_local;

            do {
                written_size_local = written_size.load(std::memory_order::relaxed);
                if (batch_size - written_size_local < size)
                    return nullptr;
            } while (!written_size.compare_exchange_weak(
                written_size_local, written_size_local + size, std::memory_order::relaxed));

            return data + written_size_local;
        }

        std::atomic<size_t> written_size = 0;
        alignas(size_t) std::byte data[batch_size]{};
    };

    Batch* pop_batch() {
        auto in  = in_.load(std::memory_order::relaxed);
        auto out = out_.load(std::memory_order::relaxed);

        auto readable = in - out;
        if (!readable)
            return nullptr;
        auto& batch = batches_[out & mask];
        if (batch.written_size.load(std::memory_order::relaxed) <= 1)
            return nullptr;

        std::atomic_signal_fence(std::memory_order_release);
        out_.store(out + 1, std::memory_order::relaxed);

        return &batch;
    }

    std::atomic<size_t> in_{0}, out_{0};
    Batch batches_[batch_count];
};

} // namespace usb