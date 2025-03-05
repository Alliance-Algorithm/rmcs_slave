#pragma once

#include <cstdint>

#include <atomic>

#include <spi.h>

#include "utility/lazy.hpp"

namespace led {

class Led {
public:
    Led() {
        reset();
    };

    void reset() {
        uplink_full_reset_counter_.store(0, std::memory_order::relaxed);
        downlink_full_reset_counter_.store(0, std::memory_order::relaxed);
        std::atomic_signal_fence(std::memory_order_release);
        user_controlling_.store(false, std::memory_order::relaxed);
    }

    // Lighting effect lasts for 5 seconds
    void uplink_buffer_full() {
        uplink_full_reset_counter_.store(5000, std::memory_order::relaxed);
    }
    void downlink_buffer_full() {
        downlink_full_reset_counter_.store(5000, std::memory_order::relaxed);
    }

    void update(uint32_t tick) {
        // Do not change the lighting when user controlling
        if (user_controlling_.load(std::memory_order::relaxed))
            return;

        // Atomic decrease counter
        uint16_t uplink_full;
        do {
            uplink_full = uplink_full_reset_counter_.load(std::memory_order::relaxed);
            if (uplink_full == 0)
                break;
        } while (!uplink_full_reset_counter_.compare_exchange_weak(
            uplink_full, uplink_full - 1, std::memory_order::relaxed));

        uint16_t downlink_full;
        do {
            downlink_full = downlink_full_reset_counter_.load(std::memory_order::relaxed);
            if (downlink_full == 0)
                break;
        } while (!downlink_full_reset_counter_.compare_exchange_weak(
            downlink_full, downlink_full - 1, std::memory_order::relaxed));

        if (uplink_full && downlink_full) {
            // Both full: yellow and aqua lights flashing alternately
            if (tick >> trigger_tick_prescaler & 0x01)
                set_value(max_light, max_light, 0);
            else
                set_value(0, max_light, max_light);
        } else if (uplink_full) {
            // Uplink full: yellow light flashing
            if (tick >> trigger_tick_prescaler & 0x01)
                set_value(max_light, max_light, 0);
            else
                set_value(0, 0, 0);
        } else if (downlink_full) {
            // Downlink full: aqua light flashing
            if (tick >> trigger_tick_prescaler & 0x01)
                set_value(0, 0, 0);
            else
                set_value(0, max_light, max_light);
        } else {
            // Working well: green light
            set_value(0, max_light, 0);
        }
    }

    void set_value(uint8_t red, uint8_t green, uint8_t blue) {
        static uint32_t color, last_color = 0;
        color = (red << 16) | (green << 8) | blue;
        if (color == last_color)
            return;
        last_color = color;

        static uint8_t txbuf[124] = {0};
        const uint8_t WS2812_HighLevel = 0xf0;
        const uint8_t WS2812_LowLevel  = 0xC0;
        for (int i = 0; i < 8; i++)
        {
            txbuf[7-i]  = (((green >> i) & 0x01) ? WS2812_HighLevel : WS2812_LowLevel)>>1;
            txbuf[15-i] = (((red >> i) & 0x01) ? WS2812_HighLevel : WS2812_LowLevel)>>1;
            txbuf[23-i] = (((blue >> i) & 0x01) ? WS2812_HighLevel : WS2812_LowLevel)>>1;
        }
        HAL_SPI_Transmit(&hspi6, (uint8_t *)txbuf, 124, 100);
    }

private:
    std::atomic<bool> user_controlling_;

    std::atomic<uint16_t> uplink_full_reset_counter_, downlink_full_reset_counter_;

    std::uint8_t max_light = 60;

    std::uint32_t trigger_tick_prescaler = 8;
};

inline utility::Lazy<Led> led;

} // namespace led