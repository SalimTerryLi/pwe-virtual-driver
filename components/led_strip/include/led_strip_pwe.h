/*
 * SPDX-FileCopyrightText: 2019-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include "pwe_io_spi.h"
#include "pwe_io_rmt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef pwe_config_t led_strip_config;

/**
* @brief Default configuration for WS2812 LED strip
*
*/
#define PWE_WS2812_CONFIG           \
    {                               \
        .T1H = 800,                 \
        .T1L = 450,                 \
        .T0H = 400,                 \
        .T0L = 850,                 \
        .T1H_ACC = 150,             \
        .T1L_ACC = 150,             \
        .T0H_ACC = 150,             \
        .T0L_ACC = 150,             \
        .TRST = 50000,              \
    }

/**
 * @brief Default configuration for SK6812 LED strip
 *
 */
#define PWE_SK6812_CONFIG           \
    {                               \
        .T1H = 600,                 \
        .T1L = 600,                 \
        .T0H = 300,                 \
        .T0L = 900,                 \
        .T1H_ACC = 150,             \
        .T1L_ACC = 150,             \
        .T0H_ACC = 150,             \
        .T0L_ACC = 150,             \
        .TRST = 80000,              \
    }

/**
 * @brief Install a new ws2812 driver (based on RMT peripheral)
 *
 * @param led_num: MAX LED number
 * @param rmt_conf: RMT periph configuration
 * @param strip: strip handle created
 *
 * @return
 *      LED strip instance or NULL
 */
esp_err_t led_strip_new_pwe_rmt(const led_strip_config *led_conf, uint16_t led_num, const rmt_config_t *rmt_conf, led_strip_handle_t *strip);

/**
 * @brief Delete a ws2812 driver (based on RMT peripheral)
 *
 * @param strip: strip handle
 *
 * @return
 *      LED strip instance or NULL
 */
esp_err_t led_strip_del_pwe_rmt(led_strip_handle_t strip);

/**
 * @brief Install a new ws2812 driver (based on SPI peripheral)
 *
 * @param led_num: MAX LED number
 * @param spi_conf: SPI periph configuration
 * @param strip: strip handle created
 *
 * @return
 *      LED strip instance or NULL
 */
esp_err_t led_strip_new_pwe_spi(const led_strip_config *led_conf, uint16_t led_num, const pwe_io_spi_config_t *spi_conf, led_strip_handle_t *strip);

/**
 * @brief Delete a ws2812 driver (based on SPI peripheral)
 *
 * @param strip: strip handle
 *
 * @return
 *      LED strip instance or NULL
 */
esp_err_t led_strip_del_pwe_spi(led_strip_handle_t strip);

#ifdef __cplusplus
}
#endif
