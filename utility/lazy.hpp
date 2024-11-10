#pragma once

#include <cstdint>
#include <tuple>

#include <algorithm>

#include "utility/assert.hpp"

namespace utility {

template <typename T, typename... Args>
class Lazy {
public:
    constexpr explicit Lazy(Args... args)
        : init_status_(InitStatus::UNINITIALIZED)
        , construction_arguments{std::move(args)...} {}

    constexpr ~Lazy() {
        if (init_status_ == InitStatus::UNINITIALIZED)
            std::destroy_at(std::addressof(construction_arguments));
        else if (init_status_ == InitStatus::INITIALIZED)
            std::destroy_at(std::addressof(object));
    }

    constexpr T* get() { return std::addressof(make_or_get_object()); }

    constexpr T* operator->() { return std::addressof(make_or_get_object()); }

    constexpr T& operator*() { return make_or_get_object(); }

    constexpr explicit operator bool() const noexcept {
        return init_status_ == InitStatus::INITIALIZED;
    }

    constexpr T* try_get() {
        if (init_status_ != InitStatus::INITIALIZED)
            return nullptr;
        return std::addressof(object);
    }

private:
    using ArgTupleT = std::tuple<Args...>;

    constexpr T& make_or_get_object() {
        if (init_status_ != InitStatus::INITIALIZED) {
            assert(init_status_ == InitStatus::UNINITIALIZED);
            init_status_ = InitStatus::INITIALIZING;

            auto moved_args = std::move(construction_arguments);
            std::destroy_at(std::addressof(construction_arguments));

            construct_object(
                std::move(moved_args), std::make_index_sequence<std::tuple_size_v<ArgTupleT>>{});

            init_status_ = InitStatus::INITIALIZED;
        }

        return object;
    }

    template <typename TupleT, std::size_t... I>
    constexpr void construct_object(TupleT&& t, std::index_sequence<I...>) {
        std::construct_at(std::addressof(object), std::get<I>(std::forward<TupleT>(t))...);
    }

    enum class InitStatus : uint8_t { UNINITIALIZED = 0, INITIALIZING = 1, INITIALIZED = 2 };
    InitStatus init_status_;

    union {
        T object;
        ArgTupleT construction_arguments;
    };
};

} // namespace utility