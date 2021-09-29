/*
 * SPDX-FileCopyrightText: 2019-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "led_strip.h"
#include "led_strip_pwe.h"
#include "pwe.h"
#include "pwe_io_rmt.h"
#include "pwe_io_spi.h"

static const char *TAG = "LED_STRIP_PWE";

typedef struct {
    led_strip_t parent;
    pwe_handle_t pwe_handle;
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;


static esp_err_t led_strip_pwe_init(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    return pwe_init(ws2812->pwe_handle);
}

static esp_err_t led_strip_pwe_deinit(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    return pwe_deinit(ws2812->pwe_handle);
}

static esp_err_t led_strip_pwe_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    ESP_RETURN_ON_FALSE(index < ws2812->strip_len, ESP_ERR_INVALID_ARG, TAG, "index out of the maximum number of leds");
    uint32_t start = index * 3;
    // In thr order of GRB
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;
    return ESP_OK;
}

static esp_err_t led_strip_pwe_refresh(led_strip_handle_t strip, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    return pwe_send(ws2812->pwe_handle, ws2812->buffer, ws2812->strip_len * 3 * 8);
}

static esp_err_t led_strip_pwe_clear(led_strip_handle_t strip, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    // Write zero to turn off all leds
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    return led_strip_pwe_refresh(strip, timeout_ms);
}

esp_err_t led_strip_new_pwe_rmt(const led_strip_config *led_conf, uint16_t led_num, const rmt_config_t *rmt_conf, led_strip_handle_t *strip)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(rmt_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL config");
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");

    // 24 bits per led
    uint32_t ws2812_size = sizeof(ws2812_t) + led_num * 3;
    ws2812_t *ws2812 = calloc(1, sizeof(ws2812_t) + ws2812_size);
    ESP_RETURN_ON_FALSE(ws2812 != NULL, ESP_ERR_NO_MEM, TAG, "Failed to alloc ws2812 handle");

    rmt_config_t rmt_config;
    memcpy(&rmt_config, rmt_conf, sizeof(rmt_config_t));
    rmt_config.clk_div = rmt_config.clk_div > 8 ? 8 : rmt_config.clk_div;   // minimum 10M
    ESP_GOTO_ON_ERROR(pwe_new_rmt_backend(led_conf, &rmt_config, 0, &ws2812->pwe_handle), err, TAG, "Failed to create pwe_rmt backend");

    ws2812->strip_len = led_num;

    ws2812->parent.init = led_strip_pwe_init;
    ws2812->parent.set_pixel = led_strip_pwe_set_pixel;
    ws2812->parent.refresh = led_strip_pwe_refresh;
    ws2812->parent.clear = led_strip_pwe_clear;
    ws2812->parent.deinit = led_strip_pwe_deinit;

    *strip = &ws2812->parent;
    return ESP_OK;
err:
    free(ws2812);
    return ret;
}

esp_err_t led_strip_del_pwe_rmt(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    ESP_RETURN_ON_ERROR(pwe_delete_rmt_backend(ws2812->pwe_handle), TAG, "Failed to delete pwe_rmt backend");
    free(ws2812);
    return ESP_OK;
}

esp_err_t led_strip_new_pwe_spi(const led_strip_config *led_conf, uint16_t led_num, const pwe_io_spi_config_t *spi_conf, led_strip_handle_t *strip)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(spi_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL config");
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");

    // 24 bits per led
    uint32_t ws2812_size = sizeof(ws2812_t) + led_num * 3;
    ws2812_t *ws2812 = calloc(1, ws2812_size);
    ESP_RETURN_ON_FALSE(ws2812 != NULL, ESP_ERR_NO_MEM, TAG, "Failed to alloc ws2812 handle");

    ESP_GOTO_ON_ERROR(pwe_new_spi_backend(led_conf, spi_conf, ws2812_size * 8, &ws2812->pwe_handle), err, TAG, "Failed to create pwe_rmt backend");

    ws2812->strip_len = led_num;

    ws2812->parent.init = led_strip_pwe_init;
    ws2812->parent.set_pixel = led_strip_pwe_set_pixel;
    ws2812->parent.refresh = led_strip_pwe_refresh;
    ws2812->parent.clear = led_strip_pwe_clear;
    ws2812->parent.deinit = led_strip_pwe_deinit;

    *strip = &ws2812->parent;
    return ESP_OK;
err:
    free(ws2812);
    return ret;
}

esp_err_t led_strip_del_pwe_spi(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    ESP_RETURN_ON_ERROR(pwe_delete_spi_backend(ws2812->pwe_handle), TAG, "Failed to delete pwe_rmt backend");
    free(ws2812);
    return ESP_OK;
}
