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

typedef struct {
    led_strip_t parent;
    pwe_handle_t pwe_handle;
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;


esp_err_t led_strip_init(led_strip_handle_t strip)
{
    return strip->init(strip);
}

esp_err_t led_strip_deinit(led_strip_handle_t strip)
{
    return strip->deinit(strip);
}

esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    return strip->set_pixel(strip, index, red, green, blue);
}

esp_err_t led_strip_refresh(led_strip_handle_t strip, uint32_t timeout_ms)
{
    return strip->refresh(strip, timeout_ms);
}

esp_err_t led_strip_clear(led_strip_handle_t strip, uint32_t timeout_ms)
{
    return strip->clear(strip, timeout_ms);
}
