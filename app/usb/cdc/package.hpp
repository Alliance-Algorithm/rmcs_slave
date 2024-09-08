#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <cstring>
#include <new>
#include <numeric>

namespace usb {

constexpr size_t kPackageMaxSize = 64;
constexpr uint8_t kPackageHead   = 0xAF;

struct Dynamic {};

template <typename T>
class __attribute__((packed)) TransmitPackage final {
public:
    TransmitPackage(uint8_t type = 0, uint8_t index = 0) noexcept {
        static_assert(sizeof(TransmitPackage) <= kPackageMaxSize);
        head_      = kPackageHead;
        type_      = type;
        index_     = index;
        data_size_ = sizeof(T);
    }

    uint8_t& type() noexcept { return type_; }
    uint8_t& index() noexcept { return index_; }
    T& data() noexcept { return data_; }

    const uint8_t* c_str() noexcept {
        uint8_t* str_this = reinterpret_cast<uint8_t*>(this);
        verify_           = std::accumulate(
            str_this, str_this + sizeof(*this) - sizeof(verify_), static_cast<uint8_t>(0));
        return str_this;
    }

    constexpr size_t size() { return sizeof(TransmitPackage); }

private:
    uint8_t head_;
    uint8_t type_;
    uint8_t index_;
    uint8_t data_size_;
    T data_;
    uint8_t verify_;
};

template <>
class __attribute__((packed)) TransmitPackage<Dynamic> final {
public:
    TransmitPackage(uint8_t type = 0, uint8_t index = 0) noexcept {
        static_assert(sizeof(TransmitPackage) <= kPackageMaxSize);
        head_      = kPackageHead;
        type_      = type;
        index_     = index;
        data_size_ = 0;
    }

    uint8_t& type() noexcept { return type_; }
    uint8_t& index() noexcept { return index_; }

    static constexpr size_t max_data_size() { return sizeof(data_and_verify_) - 1u; };

    template <typename T>
    T& data() noexcept {
        data_size_ = sizeof(T);
        return *reinterpret_cast<T*>(&data_and_verify_[0]);
    }

    void reset() { data_size_ = 0; }

    size_t append(const uint8_t* buffer, size_t size) {
        auto append_size = std::min(size, max_data_size() - data_size_);
        memcpy(data_and_verify_ + data_size_, buffer, append_size);
        data_size_ += append_size;
        return append_size;
    }

    const uint8_t* c_str() noexcept {
        uint8_t* str_this          = reinterpret_cast<uint8_t*>(this);
        size_t size_without_verify = 4u + data_size_;
        data_and_verify_[data_size_] =
            std::accumulate(str_this, str_this + size_without_verify, static_cast<uint8_t>(0));
        return str_this;
    }

    size_t size() const noexcept {
        return 4u + data_size_ + 1u;
    }

private:
    uint8_t head_;
    uint8_t type_;
    uint8_t index_;
    uint8_t data_size_;
    uint8_t data_and_verify_[kPackageMaxSize - 4];
};

class __attribute__((packed)) ReceivePackage final {
public:
    ReceivePackage() noexcept = default;

    bool verify() noexcept {
        if (head_ != kPackageHead)
            return false;

        if ((data_size_ + 1u) > sizeof(data_and_verify_))
            return false;

        uint8_t* str_this = reinterpret_cast<uint8_t*>(this);
        auto verify_ =
            std::accumulate(str_this, str_this + 4u + data_size_, static_cast<uint8_t>(0));
        if (data_and_verify_[data_size_] != verify_)
            return false;

        return true;
    }

    uint8_t type() noexcept { return type_; }
    uint8_t index() noexcept { return index_; }
    uint8_t data_size() noexcept { return data_size_; }
    uint8_t size() noexcept { return 4u + data_size_ + 1u; }

    template <typename T>
    const T& data() noexcept {
        assert(sizeof(T) == data_size_);
        return *reinterpret_cast<T*>(data_and_verify_);
    }

    uint8_t* c_str() noexcept {
        uint8_t* str_this = reinterpret_cast<uint8_t*>(this);
        return str_this;
    }

private:
    uint8_t head_;
    uint8_t type_;
    uint8_t index_;
    uint8_t data_size_;
    uint8_t data_and_verify_[kPackageMaxSize - 4];
};

} // namespace usb
