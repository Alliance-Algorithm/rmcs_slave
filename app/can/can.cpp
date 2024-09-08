#include "app/can/can.hpp"
#include "app/usb/cdc.hpp"

#include <can.h>

extern "C" {

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    auto& can_lazy = hcan == &hcan1 ? can::can1 : can::can2;
    auto field_id  = hcan == &hcan1 ? usb::field::StatusId::CAN1_ : usb::field::StatusId::CAN2_;

    if (auto can = can_lazy.try_get()) {
        if (auto cdc = usb::cdc.try_get()) {
            can->read_device_write_buffer(cdc->get_transmit_buffer(), field_id);
        }
    }
}

} // extern "C"
