#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <fdcan.h>

#include "app/usb/field.hpp"
#include "app/usb/interrupt_safe_buffer.hpp"
#include "utility/assert.hpp"
#include "utility/immovable.hpp"
#include "utility/lazy.hpp"
#include "utility/ring_buffer.hpp"

namespace can {

class Can : private utility::Immovable {
public:
    using Lazy = utility::Lazy<Can, FDCAN_HandleTypeDef*, uint32_t, uint32_t>;

    Can(FDCAN_HandleTypeDef* hal_can_handle, uint32_t hal_filter_bank,
        uint32_t hal_slave_start_filter_bank)
        : hal_can_handle_(hal_can_handle) {
        config_can(hal_filter_bank, hal_slave_start_filter_bank);
    }

    bool read_buffer_write_device(std::byte*& buffer) {
        auto construct = [&buffer](std::byte* storage) {
            auto& mailbox = *new (storage) TransmitMailboxData{};

            auto& header = *std::launder(reinterpret_cast<const FieldHeader*>(buffer));
            buffer += sizeof(header);

            // *modified from stm32h7xx_hal_fdcan.c FDCAN_CopyMessageToRAM
            uint8_t can_data_length;
            if (header.is_extended_can_id) {
                auto& ext_id = *std::launder(reinterpret_cast<const CanExtendedId*>(buffer));
                buffer += sizeof(ext_id);
                mailbox.identifier = (ext_id.can_id << 0) | FDCAN_EXTENDED_ID;
                can_data_length    = header.has_can_data ? ext_id.data_length + 1 : 0;
            } else [[likely]] {
                auto& std_id = *std::launder(reinterpret_cast<const CanStandardId*>(buffer));
                buffer += sizeof(std_id);
                mailbox.identifier = (std_id.can_id << 18) | FDCAN_STANDARD_ID;
                can_data_length    = header.has_can_data ? std_id.data_length + 1 : 0;
            }
            mailbox.identifier |= header.is_remote_transmission ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
            mailbox.data_length_and_timestamp = can_data_length;
            
            // Always read full 8 bytes to reduce the number of if-branches for performance
            // considerations (almost all CAN messages have a length of 8 bytes).
            std::memcpy(mailbox.data, buffer, 8);
            buffer += can_data_length;
        };

        if (transmit_buffer_.emplace_back_multi(construct, 1)) [[likely]] {
            return true;
        } else {
            alignas(TransmitMailboxData) std::byte dummy[sizeof(TransmitMailboxData)];
            construct(dummy);
            led::led->downlink_buffer_full();
            return false;
        }
    }

    bool try_transmit() {
        auto hcan = hal_can_handle_;

        auto state = hcan->State;

        // fetch from HAL_FDCAN_AddMessageToTxFifoQ
        assert_always(state == HAL_FDCAN_STATE_BUSY);

        uint32_t txfqs = hcan->Instance->TXFQS;
        auto free_mailbox_count = txfqs & FDCAN_TXFQS_TFFL;
        
        return transmit_buffer_.pop_front_multi(
            [this, hcan](TransmitMailboxData&& mailbox_data) {
                auto put_index = ((hcan->Instance->TXFQS & FDCAN_TXFQS_TFQPI) >> FDCAN_TXFQS_TFQPI_Pos);

                struct TxMailbox{
                    uint32_t TIR;
                    uint32_t TDTR;
                    uint32_t TDLR;
                    uint32_t TDHR;
                };
                auto target_mailbox = reinterpret_cast<TxMailbox*>(hcan->msgRam.TxBufferSA + (put_index * hcan->Init.TxElmtSize * 4U));
                
                target_mailbox->TIR = mailbox_data.identifier;
                target_mailbox->TDTR = mailbox_data.data_length_and_timestamp << 16;
                target_mailbox->TDLR = mailbox_data.data[0];
                target_mailbox->TDHR = mailbox_data.data[1];

                hcan->Instance->TXBAR = ((uint32_t)1 << put_index);
                hcan->LatestTxFifoQRequest = ((uint32_t)1 << put_index);

            },
            free_mailbox_count);
    }

private:
    friend void ::HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

    void config_can(uint32_t hal_filter_bank, uint32_t hal_slave_start_filter_bank) {
        // *midified config filter with HAL_FDCAN_ConfigFilter
        FDCAN_FilterTypeDef sFilterConfig;

        sFilterConfig.IdType = FDCAN_STANDARD_ID;
        sFilterConfig.FilterIndex           = hal_filter_bank;
        sFilterConfig.FilterType           = FDCAN_FILTER_MASK;
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        sFilterConfig.FilterID1     = 0x0000;
        sFilterConfig.FilterID2     = 0x0000;

        constexpr auto ok = HAL_OK;
        assert_always(HAL_FDCAN_ConfigFilter(hal_can_handle_, &sFilterConfig) == ok);

        sFilterConfig.IdType = FDCAN_EXTENDED_ID;
        sFilterConfig.FilterIndex           = hal_filter_bank+1;
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

        assert_always(HAL_FDCAN_ConfigFilter(hal_can_handle_, &sFilterConfig) == ok);
        
        assert_always(HAL_FDCAN_Start(hal_can_handle_) == ok);
        assert_always(HAL_FDCAN_ActivateNotification(hal_can_handle_, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) == ok);
    }

    bool read_device_write_buffer(
        usb::InterruptSafeBuffer& buffer_wrapper, usb::field::UplinkId field_id) {
        auto hal_can_state    = hal_can_handle_->State;
        auto hal_can_instance = hal_can_handle_->Instance;

        // modified from HAL_FDCAN_GetRxMessage
        assert_always(hal_can_state == HAL_FDCAN_STATE_BUSY);
        assert_always((hal_can_instance->RXF0S & FDCAN_RXF0S_F0FL) != 0U); // Assert if rx_fifo is empty

        struct RxMailbox{
            uint32_t RIR;
            uint32_t RDTR;
            uint32_t RDLR;
            uint32_t RDHR;
        };
        auto get_index = ((hal_can_instance->RXF0S & FDCAN_RXF0S_F0GI) >> FDCAN_RXF0S_F0GI_Pos);
        auto rx_mailbox = reinterpret_cast<RxMailbox*>(hal_can_handle_->msgRam.RxFIFO0SA + (get_index * hal_can_handle_->Init.TxElmtSize * 4U));

        bool is_extended_can_id     = static_cast<bool>(rx_mailbox->RIR & 0x40000000U);
        bool is_remote_transmission = static_cast<bool>(rx_mailbox->RIR & 0x20000000U);
        size_t can_data_length      = (rx_mailbox->RDTR & 0x000F0000U) >> 16;

        // Calculate field size and try to allocate from buffer
        std::byte* buffer = buffer_wrapper.allocate(
            sizeof(FieldHeader)
            + (is_extended_can_id ? sizeof(CanExtendedId) : sizeof(CanStandardId))
            + can_data_length);

        if (buffer) {
            // Write field header
            auto& header = *new (buffer) FieldHeader{};
            buffer += sizeof(FieldHeader);
            header.field_id               = static_cast<uint8_t>(field_id);
            header.is_extended_can_id     = is_extended_can_id;
            header.is_remote_transmission = is_remote_transmission;
            header.has_can_data           = static_cast<bool>(can_data_length);

            // Write CAN id and data length
            if (is_extended_can_id) {
                auto& ext_id = *new (buffer) CanExtendedId{};
                buffer += sizeof(CanExtendedId);
                ext_id.can_id = rx_mailbox->RIR & 0x1FFFFFFFU;
                ext_id.data_length = can_data_length - 1;
            } else [[likely]] {
                auto& std_id = *new (buffer) CanStandardId{};
                buffer += sizeof(CanStandardId);
                std_id.can_id      = (rx_mailbox->RIR & 0x1FFC0000U) >> 18;
                std_id.data_length = can_data_length - 1;
            }

            // Write CAN data
            uint32_t can_data[2];
            can_data[0] = rx_mailbox->RDLR;
            can_data[1] = rx_mailbox->RDHR;
            std::memcpy(buffer, can_data, can_data_length);
            buffer += can_data_length;
        }

        // Release the FIFO
        hal_can_handle_->Instance->RXF0A = get_index;

        return static_cast<bool>(buffer);
    }

    FDCAN_HandleTypeDef* hal_can_handle_;

    struct __attribute__((packed)) FieldHeader {
        uint8_t field_id            : 4;
        bool is_extended_can_id     : 1;
        bool is_remote_transmission : 1;
        bool has_can_data           : 1;
    };

    struct __attribute__((packed)) CanStandardId {
        uint32_t can_id     : 11;
        uint8_t data_length : 3;
    };

    struct __attribute__((packed)) CanExtendedId {
        uint32_t can_id     : 29;
        uint8_t data_length : 3;
    };

    struct TransmitMailboxData {
        uint32_t identifier;                // CAN_TxMailBox_TypeDef::TIR
        uint32_t data_length_and_timestamp; // CAN_TxMailBox_TypeDef::TDTR
        uint32_t data[2];                   // CAN_TxMailBox_TypeDef::TDLR & TDHR
    };
    utility::RingBuffer<TransmitMailboxData, 16> transmit_buffer_;
};

inline constinit Can::Lazy can1{&hfdcan1, 0, 14};
inline constinit Can::Lazy can2{&hfdcan2, 14, 14};
inline constinit Can::Lazy can3{&hfdcan3, 28, 14};

} // namespace can
