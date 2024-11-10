#include "app/can/can.hpp"
#include "app/usb/cdc.hpp"

#include <can.h>

extern "C" {

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    auto can      = hcan == &hcan1 ? can::can1.get() : can::can2.get();
    auto field_id = hcan == &hcan1 ? usb::field::UplinkId::CAN1_ : usb::field::UplinkId::CAN2_;

    can->read_device_write_buffer(usb::cdc->get_transmit_buffer(), field_id);
}

} // extern "C"
