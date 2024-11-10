#pragma once

#include <cstddef>

#include <algorithm>
#include <atomic>
#include <bit>

namespace utility {

template <typename T, size_t max_size>
requires(max_size >= 2 && std::has_single_bit(max_size))
class RingBuffer {
public:
    constexpr RingBuffer() = default;

    ~RingBuffer() { clear(); }

    /*!
     * \brief Check how many elements can be read from the buffer
     * \return Number of elements that can be read
     */
    size_t readable() const {
        return in_.load(std::memory_order_relaxed) - out_.load(std::memory_order_relaxed);
    }

    /*!
     * \brief Check how many elements can be written into the buffer
     * \return Number of free slots that can be be written
     */
    size_t writeable() const { return max_size - readable(); }

    template <typename F>
    requires requires(F f, std::byte* storage) { f(storage); }
    size_t emplace_back_multi(F construct_functor, size_t count = max_size) {
        auto in  = in_.load(std::memory_order::relaxed);
        auto out = out_.load(std::memory_order::relaxed);

        auto writeable = max_size - (in - out);

        if (count > writeable)
            count = writeable;
        if (!count)
            return 0;

        auto offset = in & mask;
        auto slice  = std::min(count, max_size - offset);

        for (size_t i = 0; i < slice; i++)
            construct_functor(storage_[offset + i].data);
        for (size_t i = 0; i < count - slice; i++)
            construct_functor(storage_[i].data);

        std::atomic_signal_fence(std::memory_order_release);
        in_.store(in + count, std::memory_order::relaxed);

        return count;
    }

    template <typename... Args>
    bool emplace_back(Args&&... args) {
        return emplace_back_multi(
            [&](std::byte* storage) { new (storage) T{std::forward<Args...>(args...)}; }, 1);
    }

    template <typename F>
    requires requires(F f, T t) { f(std::move(t)); }
    size_t pop_front_multi(F callback_functor, size_t count = max_size) {
        auto in  = in_.load(std::memory_order::relaxed);
        auto out = out_.load(std::memory_order::relaxed);

        auto readable = in - out;
        if (count > readable)
            count = readable;
        if (!count)
            return 0;

        auto offset = out & mask;
        auto slice  = std::min(count, max_size - offset);

        auto process = [&callback_functor](std::byte* storage) {
            auto& element = *std::launder(reinterpret_cast<T*>(storage));
            callback_functor(std::move(element));
            std::destroy_at(&element);
        };
        for (size_t i = 0; i < slice; i++)
            process(storage_[offset + i].data);
        for (size_t i = 0; i < count - slice; i++)
            process(storage_[i].data);

        std::atomic_signal_fence(std::memory_order_release);
        out_.store(out + count, std::memory_order::relaxed);

        return count;
    }

    template <typename F>
    requires requires(F f, T t) { f(std::move(t)); }
    bool pop_front(F&& callback_functor) {
        return pop_front_multi(std::forward<F>(callback_functor), 1);
    }

    /*!
     * \brief Clear buffer
     * \return Number of elements that be erased
     */
    size_t clear() {
        return pop_front_multi([](T&&) {});
    }

private:
    static constexpr size_t mask = max_size - 1;

    std::atomic<size_t> in_{0}, out_{0};
    struct {
        alignas(T) std::byte data[sizeof(T)];
    } storage_[max_size]{};
};

}; // namespace utility
