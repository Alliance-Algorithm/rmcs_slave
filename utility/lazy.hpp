#pragma once

#include <atomic>
#include <tuple>

#include "utility/assert.hpp"

namespace utility {

template <typename T, typename... Args>
class Lazy {
public:
    constexpr explicit Lazy(Args... args)
        : init_status_(InitStatus::UNINITIALIZED)
        , construction_arguments{std::move(args)...} {}

    constexpr ~Lazy(){}; // No need to deconstruct

    constexpr T& init() {
        auto init_status = init_status_.load(std::memory_order::relaxed);
        if (init_status != InitStatus::INITIALIZED) {
            assert(init_status == InitStatus::UNINITIALIZED);
            init_status_.store(InitStatus::INITIALIZING, std::memory_order::relaxed);

            auto moved_args = std::move(construction_arguments);
            std::destroy_at(std::addressof(construction_arguments));

            construct_object(
                std::move(moved_args), std::make_index_sequence<std::tuple_size_v<ArgTupleT>>{});

            init_status_.store(InitStatus::INITIALIZED, std::memory_order::relaxed);
        }

        return object;
    }

    constexpr T* get() {
        assert(*this);
        return std::addressof(object);
    }

    constexpr T* operator->() {
        assert(*this);
        return std::addressof(object);
    }

    constexpr T& operator*() {
        assert(*this);
        return object;
    }

    constexpr explicit operator bool() const noexcept {
        return init_status_.load(std::memory_order::relaxed) == InitStatus::INITIALIZED;
    }

private:
    using ArgTupleT = std::tuple<Args...>;

    template <typename TupleT, std::size_t... I>
    constexpr void construct_object(TupleT&& t, std::index_sequence<I...>) {
        std::construct_at(std::addressof(object), std::get<I>(std::forward<TupleT>(t))...);
    }

    enum class InitStatus : uint8_t { UNINITIALIZED = 2, INITIALIZING = 1, INITIALIZED = 0 };
    std::atomic<InitStatus> init_status_;

    union {
        T object;
        ArgTupleT construction_arguments;
    };
};

} // namespace utility