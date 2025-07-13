#include "app/can/can.hpp"
#include "app/usb/cdc.hpp"

#include <fdcan.h>

extern "C" {

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    auto can      = hfdcan == &hfdcan1 ? can::can1.get() : (hfdcan == &hfdcan2 ? can::can2.get() : can::can3.get());
    auto field_id  = hfdcan == &hfdcan1 ? usb::field::UplinkId::CAN1_ : 
                                (hfdcan == &hfdcan2 ? usb::field::UplinkId::CAN2_ : usb::field::UplinkId::CAN3_);

    can->read_device_write_buffer(usb::cdc->get_transmit_buffer(), field_id);
}

} // extern "C"
