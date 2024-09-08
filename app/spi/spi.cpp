#include "app/spi/spi.hpp"

#include <spi.h>

extern "C" {

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi1) {
        if (auto spi = spi::spi1.try_get())
            spi->transmit_receive_callback();
    }
}

} // extern "C"