/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pwe.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t gpio;
    spi_host_device_t spi_bus;
} pwe_io_spi_config_t;

/**
 * @brief Create PWE interface with SPI host driver
 *
 * @param config: PWE configuration
 * @param spi_conf: SPI config
 * @param buffer_size: maximum length that will be sent, bits
 * @param handle: filled with created handle
 *
 * @note The actual outgoing buffer size that is required by the driver differs.
 *       buffer_size should be the maximum bit count that later this driver can consume
 *
 * @return
 *      PWE instance or NULL
 */
esp_err_t pwe_new_spi_backend(const pwe_config_t *config, const pwe_io_spi_config_t *spi_conf, uint32_t buffer_size, pwe_handle_t *handle);

/**
 * @brief Delete SPI based PWE interface
 *
 * @param config: handle
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_delete_spi_backend(pwe_handle_t handle);

#ifdef __cplusplus
}
#endif
