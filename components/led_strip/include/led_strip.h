/*
 * SPDX-FileCopyrightText: 2019-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
* @brief LED Strip Type
*
*/
typedef struct led_strip_s led_strip_t;

/**
* @brief LED Strip Type
*
*/
typedef led_strip_t *led_strip_handle_t;

/**
* @brief Declare of LED Strip Type
*
*/
struct led_strip_s {
    esp_err_t (*init)(led_strip_handle_t strip);
    esp_err_t (*set_pixel)(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
    esp_err_t (*refresh)(led_strip_handle_t strip, uint32_t timeout_ms);
    esp_err_t (*clear)(led_strip_handle_t strip, uint32_t timeout_ms);
    esp_err_t (*deinit)(led_strip_handle_t strip);
};

/**
 * @brief Init the LED strip configuration.
 *
 * @param strip: LED strip
 *
 * @return
 *      LED strip instance or NULL
 */
esp_err_t led_strip_init(led_strip_handle_t strip);

/**
 * @brief Denit the RMT peripheral.
 *
 * @param[in] strip: LED strip
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t led_strip_denit(led_strip_handle_t strip);

/**
    * @brief Set RGB for a specific pixel
    *
    * @param strip: LED strip
    * @param index: index of pixel to set
    * @param red: red part of color
    * @param green: green part of color
    * @param blue: blue part of color
    *
    * @return
    *      - ESP_OK: Set RGB for a specific pixel successfully
    *      - ESP_ERR_INVALID_ARG: Set RGB for a specific pixel failed because of invalid parameters
    *      - ESP_FAIL: Set RGB for a specific pixel failed because other error occurred
    */
esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

/**
    * @brief Refresh memory colors to LEDs
    *
    * @param strip: LED strip
    * @param timeout_ms: timeout value for refreshing task
    *
    * @return
    *      - ESP_OK: Refresh successfully
    *      - ESP_ERR_TIMEOUT: Refresh failed because of timeout
    *      - ESP_FAIL: Refresh failed because some other error occurred
    *
    * @note:
    *      After updating the LED colors in the memory, a following invocation of this API is needed to flush colors to strip.
    */
esp_err_t led_strip_refresh(led_strip_handle_t strip, uint32_t timeout_ms);

/**
    * @brief Clear LED strip (turn off all LEDs)
    *
    * @param strip: LED strip
    * @param timeout_ms: timeout value for clearing task
    *
    * @return
    *      - ESP_OK: Clear LEDs successfully
    *      - ESP_ERR_TIMEOUT: Clear LEDs failed because of timeout
    *      - ESP_FAIL: Clear LEDs failed because some other error occurred
    */
esp_err_t led_strip_clear(led_strip_handle_t strip, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
