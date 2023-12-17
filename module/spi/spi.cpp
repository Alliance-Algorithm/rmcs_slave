#include "spi.hpp"

#include <spi.h>

extern "C" {

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi1) {
        module::spi::spi1.weak_execute(
            [](module::spi::Spi* spi) { spi->transmit_receive_callback(); });
    }
}

} // extern "C"