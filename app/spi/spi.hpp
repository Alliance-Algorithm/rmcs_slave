#pragma once

#include <cstdio>

#include <optional>

#include <spi.h>
#include <usbd_cdc.h>

#include "utility/assert.hpp"
#include "utility/immovable.hpp"
#include "utility/lazy.hpp"

namespace spi {

class SpiModuleInterface {
public:
    friend class Spi;
    SpiModuleInterface(GPIO_TypeDef* _chip_select_port, uint16_t _chip_select_pin)
        : chip_select_port(_chip_select_port)
        , chip_select_pin(_chip_select_pin) {}

protected:
    virtual void transmit_receive_callback(uint8_t* rx_buffer, size_t size) = 0;

    GPIO_TypeDef* const chip_select_port;
    const uint16_t chip_select_pin;
};

enum class SpiTransmitReceiveMode { BLOCK, INTERRUPT };

class Spi : private utility::Immovable {
public:
    using Lazy = utility::Lazy<Spi, SPI_HandleTypeDef*>;

    explicit Spi(SPI_HandleTypeDef* hal_spi_handle)
        : hal_spi_handle_(hal_spi_handle)
        , task_created_(false)
        , spi_module_(nullptr)
        , tx_rx_size_(0) {}

    template <SpiTransmitReceiveMode mode>
    class TransmitReceiveTask {
    public:
        friend class Spi;

        TransmitReceiveTask(const TransmitReceiveTask&)            = delete;
        TransmitReceiveTask& operator=(const TransmitReceiveTask&) = delete;

        TransmitReceiveTask(TransmitReceiveTask&& task)
            : tx_buffer(task.tx_buffer)
            , spi_(task.spi_) {
            task.spi_ = nullptr;
        }
        TransmitReceiveTask& operator=(TransmitReceiveTask&& task) = delete;

        ~TransmitReceiveTask() {
            if (spi_ != nullptr) {
                spi_->start_transmit_receive();

                if constexpr (mode == SpiTransmitReceiveMode::BLOCK) {
                    HAL_SPI_TransmitReceive(
                        spi_->hal_spi_handle_, spi_->tx_data_buffer_, spi_->rx_data_buffer_,
                        spi_->tx_rx_size_, HAL_MAX_DELAY);
                    spi_->transmit_receive_callback();
                } else if constexpr (mode == SpiTransmitReceiveMode::INTERRUPT) {
                    HAL_SPI_TransmitReceive_IT(
                        spi_->hal_spi_handle_, spi_->tx_data_buffer_, spi_->rx_data_buffer_,
                        spi_->tx_rx_size_);
                }

                spi_->task_created_ = false;
            }
        }

        uint8_t* tx_buffer;

    private:
        explicit TransmitReceiveTask(Spi* spi)
            : tx_buffer(spi->tx_data_buffer_)
            , spi_(spi) {}

        Spi* spi_;
    };

    template <SpiTransmitReceiveMode mode>
    std::optional<TransmitReceiveTask<mode>>
        create_transmit_receive_task(SpiModuleInterface* module, size_t size) {

        assert(max_buffer_size_ >= size);

        if (__sync_bool_compare_and_swap(&task_created_, false, true)) {
            if (hal_ready()) {
                // The function transmit_receive_callback has a very low probability of not being
                // called. Therefore, before creating a new task, release the chip select pin again
                // to ensure correct data transmission.
                if (spi_module_)
                    stop_transmit_receive();

                spi_module_ = module;
                tx_rx_size_ = size;
                return TransmitReceiveTask<mode>(this);
            } else {
                task_created_ = false;
            }
        }
        return std::nullopt;
    }

private:
    friend void ::HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef*);

    void transmit_receive_callback() {
        stop_transmit_receive();

        spi_module_->transmit_receive_callback(rx_data_buffer_, tx_rx_size_);
    }

    bool hal_ready() const { return hal_spi_handle_->State == HAL_SPI_STATE_READY; }

    void start_transmit_receive() {
        HAL_GPIO_WritePin(
            spi_module_->chip_select_port, spi_module_->chip_select_pin, GPIO_PIN_RESET);
    }

    void stop_transmit_receive() {
        HAL_GPIO_WritePin(
            spi_module_->chip_select_port, spi_module_->chip_select_pin, GPIO_PIN_SET);
    }

    SPI_HandleTypeDef* hal_spi_handle_;

    bool task_created_;

    SpiModuleInterface* spi_module_;
    size_t tx_rx_size_;

    static constexpr size_t max_buffer_size_ = 16;
    alignas(4) uint8_t tx_data_buffer_[max_buffer_size_];
    alignas(4) uint8_t rx_data_buffer_[max_buffer_size_];
};

inline constinit Spi::Lazy spi1(&hspi1);

} // namespace spi
