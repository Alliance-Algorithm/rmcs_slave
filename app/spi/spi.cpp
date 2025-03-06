#include "app/spi/spi.hpp"

#include <spi.h>

extern "C" {

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2) {
        spi::spi1->transmit_receive_callback();
    }
}

} // extern "C"