#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <can.h>

#include "app/usb/field.hpp"
#include "app/usb/interrupt_safe_buffer.hpp"
#include "utility/assert.hpp"
#include "utility/immovable.hpp"
#include "utility/lazy.hpp"
#include "utility/ring_buffer.hpp"

namespace can {

class Can : private utility::Immovable {
public:
    using Lazy = utility::Lazy<Can, CAN_HandleTypeDef*, uint32_t, uint32_t>;

    Can(CAN_HandleTypeDef* hal_can_handle, uint32_t hal_filter_bank,
        uint32_t hal_slave_start_filter_bank)
        : hal_can_handle_(hal_can_handle) {
        config_can(hal_filter_bank, hal_slave_start_filter_bank);
    }

    bool read_buffer_write_device(std::byte*& buffer) {
        auto construct = [&buffer](std::byte* storage) {
            auto& mailbox = *new (storage) TransmitMailboxData{};

            auto& header = *std::launder(reinterpret_cast<const FieldHeader*>(buffer));
            buffer += sizeof(header);

            uint8_t can_data_length;
            if (header.is_extended_can_id) {
                auto& ext_id = *std::launder(reinterpret_cast<const CanExtendedId*>(buffer));
                buffer += sizeof(ext_id);
                mailbox.identifier = (ext_id.can_id << CAN_TI0R_EXID_Pos) | CAN_ID_EXT;
                can_data_length    = header.has_can_data ? ext_id.data_length + 1 : 0;
            } else [[likely]] {
                auto& std_id = *std::launder(reinterpret_cast<const CanStandardId*>(buffer));
                buffer += sizeof(std_id);
                mailbox.identifier = (std_id.can_id << CAN_TI0R_STID_Pos) | CAN_ID_STD;
                can_data_length    = header.has_can_data ? std_id.data_length + 1 : 0;
            }
            mailbox.identifier |= header.is_remote_transmission ? CAN_RTR_REMOTE : CAN_RTR_DATA;
            mailbox.identifier |= CAN_TI0R_TXRQ;
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
            return false;
        }
    }

    bool try_transmit() {
        auto hcan = hal_can_handle_;

        auto state = hcan->State;
        assert_always((state == HAL_CAN_STATE_READY) || (state == HAL_CAN_STATE_LISTENING));

        uint32_t tsr = hcan->Instance->TSR;
        auto free_mailbox_count =
            !!(tsr & CAN_TSR_TME0) + !!(tsr & CAN_TSR_TME1) + !!(tsr & CAN_TSR_TME2);

        return transmit_buffer_.pop_front_multi(
            [this, hcan](TransmitMailboxData&& mailbox_data) {
                auto target_mailbox_index =
                    (hcan->Instance->TSR & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;
                assert_always(target_mailbox_index <= 2);

                auto& target_mailbox = hal_can_handle_->Instance->sTxMailBox[target_mailbox_index];
                target_mailbox.TDTR  = mailbox_data.data_length_and_timestamp;
                target_mailbox.TDLR  = mailbox_data.data[0];
                target_mailbox.TDHR  = mailbox_data.data[1];
                target_mailbox.TIR   = mailbox_data.identifier;
            },
            free_mailbox_count);
    }

private:
    friend void ::HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef*);

    void config_can(uint32_t hal_filter_bank, uint32_t hal_slave_start_filter_bank) {
        CAN_FilterTypeDef sFilterConfig;

        sFilterConfig.FilterBank           = hal_filter_bank;
        sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
        sFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
        sFilterConfig.FilterIdHigh         = 0x0000;
        sFilterConfig.FilterIdLow          = 0x0000;
        sFilterConfig.FilterMaskIdHigh     = 0x0000;
        sFilterConfig.FilterMaskIdLow      = 0x0000;
        sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        sFilterConfig.FilterActivation     = CAN_FILTER_ENABLE;
        sFilterConfig.SlaveStartFilterBank = hal_slave_start_filter_bank;

        constexpr auto ok = HAL_OK;
        assert_always(HAL_CAN_ConfigFilter(hal_can_handle_, &sFilterConfig) == ok);
        assert_always(HAL_CAN_Start(hal_can_handle_) == ok);
        assert_always(
            HAL_CAN_ActivateNotification(hal_can_handle_, CAN_IT_RX_FIFO0_MSG_PENDING) == ok);
    }

    bool read_device_write_buffer(
        usb::InterruptSafeBuffer& buffer_wrapper, usb::field::StatusId field_id) {
        auto hal_can_state    = hal_can_handle_->State;
        auto hal_can_instance = hal_can_handle_->Instance;

        assert_always(
            (hal_can_state == HAL_CAN_STATE_READY) || (hal_can_state == HAL_CAN_STATE_LISTENING));
        assert_always((hal_can_instance->RF0R & CAN_RF0R_FMP0) != 0U); // Assert if rx_fifo is empty

        auto hal_can_instance_rir  = hal_can_instance->sFIFOMailBox[CAN_RX_FIFO0].RIR;
        auto hal_can_instance_rdtr = hal_can_instance->sFIFOMailBox[CAN_RX_FIFO0].RDTR;

        bool is_extended_can_id     = static_cast<bool>(CAN_RI0R_IDE & hal_can_instance_rir);
        bool is_remote_transmission = static_cast<bool>(CAN_RI0R_RTR & hal_can_instance_rir);
        size_t can_data_length      = (CAN_RDT0R_DLC & hal_can_instance_rdtr) >> CAN_RDT0R_DLC_Pos;

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
                ext_id.can_id =
                    ((CAN_RI0R_EXID | CAN_RI0R_STID) & hal_can_instance_rir) >> CAN_RI0R_EXID_Pos;
                ext_id.data_length = can_data_length - 1;
            } else [[likely]] {
                auto& std_id = *new (buffer) CanStandardId{};
                buffer += sizeof(CanStandardId);
                std_id.can_id      = (CAN_RI0R_STID & hal_can_instance_rir) >> CAN_TI0R_STID_Pos;
                std_id.data_length = can_data_length - 1;
            }

            // Write CAN data
            uint32_t can_data[2];
            can_data[0] = hal_can_instance->sFIFOMailBox[CAN_RX_FIFO0].RDLR;
            can_data[1] = hal_can_instance->sFIFOMailBox[CAN_RX_FIFO0].RDHR;
            std::memcpy(buffer, can_data, can_data_length);
            buffer += can_data_length;
        }

        // Release the FIFO
        hal_can_instance->RF0R |= CAN_RF0R_RFOM0;

        return static_cast<bool>(buffer);
    }

    CAN_HandleTypeDef* hal_can_handle_;

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

inline constinit Can::Lazy can1{&hcan1, 0, 14};
inline constinit Can::Lazy can2{&hcan2, 14, 14};

} // namespace can
