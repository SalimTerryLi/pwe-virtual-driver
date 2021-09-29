/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pwe.h"
#include "driver/rmt.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create PWE interface with RMT driver
 *
 * @param config: PWE configuration
 * @param rmt_conf: RMT configuration
 * @param buffer_size: maximum length that will be sent, bits
 * @param handle: filled with created handle
 *
 * @note The actual outgoing buffer size that is required by the driver differs.
 *       buffer_size should be the maximum bit count that later this driver can consume
 *
 * @return
 *      PWE instance or NULL
 */
esp_err_t pwe_new_rmt_backend(const pwe_config_t *config, const rmt_config_t *rmt_conf, uint32_t buffer_size, pwe_handle_t *handle);

/**
 * @brief Delete RMT based PWE interface
 *
 * @param config: handle
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_delete_rmt_backend(pwe_handle_t handle);

#ifdef __cplusplus
}
#endif
