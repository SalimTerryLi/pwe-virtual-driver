/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "pwe_io_spi.h"
#include "esp_check.h"

static const char *TAG = "PWE_IO_SPI";

#define UINTROUNDDIV(divd, divor) ( ((divd) + ((divor) / 2)) / (divor) )
#define UINTCEILDIV(divd, divor) ( ((divd) + (divor) - 1) / (divor) )

static inline uint32_t calc_aligned_error(uint32_t divided, uint32_t divisor)
{
    return abs((int)(divided - UINTROUNDDIV(divided, divisor) * divisor));
}

static uint32_t find_suitable_factor(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint16_t range)
{
    uint32_t min_val = a < b ? a : b;
    min_val = min_val < c ? min_val : c;
    min_val = min_val < d ? min_val : d;
    uint32_t max_val = a > b ? a : b;
    min_val = max_val > c ? max_val : c;
    min_val = max_val > d ? max_val : d;
    uint32_t current_factor = 0;
    uint16_t current_error = range + 1;
    uint16_t current_multiple = 0;
    for (uint32_t num = min_val + range - 1; num > 1; --num) {
        uint16_t a_align_err = calc_aligned_error(a, num);
        uint16_t b_align_err = calc_aligned_error(b, num);
        uint16_t c_align_err = calc_aligned_error(c, num);
        uint16_t d_align_err = calc_aligned_error(d, num);
        uint32_t max_align_err = a_align_err > b_align_err ? a_align_err : b_align_err;
        max_align_err = max_align_err > c_align_err ? max_align_err : c_align_err;
        max_align_err = max_align_err > d_align_err ? max_align_err : d_align_err;
        if (max_align_err < range) {
            uint16_t temp_multiple = UINTROUNDDIV(max_val, num);
            if (temp_multiple > 4) {
                // stop here, as following results are not practical to use
                break;
            }
            // only update the 'best' result if there is significant reduced error but without increasing multiple too much
            if ((max_align_err < current_error && current_multiple <= temp_multiple) ||
                    (max_align_err < current_error / 2 && temp_multiple > current_multiple)) {
                current_factor = num;
                current_error = max_align_err;
                current_multiple = temp_multiple;
            }
        }
    }
    return current_factor;
}

typedef struct {
    struct pwe_s base;
    pwe_io_spi_config_t spi_conf;
    spi_device_handle_t iohdl;
    uint32_t sclk;
    uint8_t t1h;
    uint8_t t1l;
    uint8_t t0h;
    uint8_t t0l;
    uint32_t trst;
    uint32_t buffer_size;
    uint8_t buffer[0];
} pwe_io_spi_handle_t;

static esp_err_t pwe_io_spi_init(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    spi_bus_config_t buscfg = {
        .mosi_io_num = pwe_spi->spi_conf.gpio,
        .miso_io_num = -1,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = UINTCEILDIV(pwe_spi->base.max_payload_length, 8),
    };
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = pwe_spi->sclk,
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 4,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(pwe_spi->spi_conf.spi_bus, &buscfg, SPI_DMA_CH_AUTO), TAG, "Failed to initialize spi_bus");
    ESP_RETURN_ON_ERROR(spi_bus_add_device(pwe_spi->spi_conf.spi_bus, &devcfg, &pwe_spi->iohdl), TAG, "Failed to add spi device");
    return ESP_OK;
}

static esp_err_t pwe_io_spi_deinit(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    ESP_RETURN_ON_ERROR(spi_bus_remove_device(pwe_spi->iohdl), TAG, "Failed to remove spi device");
    ESP_RETURN_ON_ERROR(spi_bus_free(pwe_spi->spi_conf.spi_bus), TAG, "Failed to free spi bus");
    return ESP_OK;
}

static esp_err_t pwe_io_spi_convert_buffer(pwe_handle_t handle, const void *data, uint32_t len, uint32_t *outgoing_buffer_len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    ESP_RETURN_ON_FALSE(pwe_spi->base.max_payload_length >= len, ESP_ERR_INVALID_ARG, TAG, "len too big");
    memset(pwe_spi->buffer, 0, UINTCEILDIV(pwe_spi->buffer_size, 8));
    uint32_t bits_src_proceeded = 0;
    uint8_t overflow_byte = 0x00;
    uint32_t bit_offset_dest = 0;
    uint32_t byte_offset_dest = 0;
    while (bits_src_proceeded < len) {
        uint32_t bit_offset_src = bits_src_proceeded % 8;
        uint32_t byte_offset_src = bits_src_proceeded / 8;
        bool src_bit_val = ((uint8_t *)data)[byte_offset_src] & (1 << (7 - bit_offset_src)); // input MSBit first
        uint16_t temp_buffer = (pwe_spi->buffer[byte_offset_dest] << 8);
        if (bit_offset_dest >= 8) { // overflow detected, switch to next byte
            temp_buffer = overflow_byte << 8;
            bit_offset_dest %= 8;
            ++byte_offset_dest;
        }
        if (src_bit_val) {
            temp_buffer |= (uint16_t)(((uint8_t)(0xff << (8 - pwe_spi->t1h)) << 8) >> bit_offset_dest);
            bit_offset_dest += pwe_spi->t1h;
            temp_buffer &= ~(uint16_t)(((uint8_t)(0xff << (8 - pwe_spi->t1l)) << 8) >> bit_offset_dest);
            bit_offset_dest += pwe_spi->t1l;
        } else {
            temp_buffer |= (uint16_t)(((uint8_t)(0xff << (8 - pwe_spi->t0h)) << 8) >> bit_offset_dest);
            bit_offset_dest += pwe_spi->t0h;
            temp_buffer &= ~(uint16_t)(((uint8_t)(0xff << (8 - pwe_spi->t0l)) << 8) >> bit_offset_dest);
            bit_offset_dest += pwe_spi->t0l;
        }
        pwe_spi->buffer[byte_offset_dest] = ((uint8_t *)(&temp_buffer))[1];
        overflow_byte = ((uint8_t *)(&temp_buffer))[0];
        ++bits_src_proceeded;
    }
    uint32_t bits_dest_filled = byte_offset_dest * 8 + bit_offset_dest;
    ESP_LOGD(TAG, "bits_dest_filled: %u", bits_dest_filled);
    *outgoing_buffer_len = bits_dest_filled;
    return ESP_OK;
}

static esp_err_t pwe_io_spi_write(pwe_handle_t handle, uint32_t len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len;
    t.tx_buffer = pwe_spi->buffer;
    t.rx_buffer = NULL;
    ESP_RETURN_ON_ERROR(spi_device_transmit(pwe_spi->iohdl, &t), TAG, "transmit SPI samples failed");
    return ESP_OK;
}

static esp_err_t pwe_io_spi_ensure_rst(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    uint32_t delay_us = pwe_spi->trst / 1000;
    delay_us = delay_us == 0 ? 1 : delay_us;
    esp_rom_delay_us(delay_us);
    return ESP_OK;
}

esp_err_t pwe_new_spi_backend(const pwe_config_t *config, const pwe_io_spi_config_t *spi_conf, uint32_t buffer_size, pwe_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "null config");

    pwe_io_spi_handle_t temp_conf;
    temp_conf.trst = config->TRST;

    // calc required pulse width pattern at first
    uint32_t accepted_range = (config->T1H_ACC + config->T1L_ACC + config->T0H_ACC + config->T0L_ACC) / 4;
    uint32_t period_per_slot_ns = find_suitable_factor(config->T1H, config->T1L, config->T0H, config->T0L, accepted_range);
    ESP_LOGD(TAG, "period_per_slot_ns: %u", period_per_slot_ns);
    ESP_RETURN_ON_FALSE(period_per_slot_ns != 0, ESP_ERR_INVALID_ARG, TAG, "Cannot resolve requested timing");
    // calc sclk
    temp_conf.sclk = 1000000000 / period_per_slot_ns;
    temp_conf.t1h = UINTROUNDDIV(config->T1H, period_per_slot_ns);
    temp_conf.t1l = UINTROUNDDIV(config->T1L, period_per_slot_ns);
    temp_conf.t0h = UINTROUNDDIV(config->T0H, period_per_slot_ns);
    temp_conf.t0l = UINTROUNDDIV(config->T0L, period_per_slot_ns);
    ESP_LOGD(TAG, "slot configuration: t1h=%u, t1l=%u, t0h=%u, t0l=%u", temp_conf.t1h, temp_conf.t1l, temp_conf.t0h, temp_conf.t0l);

    // alloc memory at the end
    uint16_t max_slots_per_bit = (temp_conf.t0h + temp_conf.t0l > temp_conf.t1h + temp_conf.t1l) ?
                                 temp_conf.t0h + temp_conf.t0l :
                                 temp_conf.t1h + temp_conf.t1l;
    temp_conf.buffer_size = max_slots_per_bit * buffer_size;
    ESP_LOGD(TAG, "Will allocate outgoing buffer with %u bits, =%u bytes", temp_conf.buffer_size, UINTCEILDIV(temp_conf.buffer_size, 8));
    pwe_io_spi_handle_t *pwe_spi = heap_caps_calloc(1, sizeof(pwe_io_spi_handle_t) + UINTCEILDIV(temp_conf.buffer_size, 8), MALLOC_CAP_DMA);
    ESP_RETURN_ON_FALSE(pwe_spi != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate pwe_io_spi_handle_t");
    memcpy(&temp_conf.spi_conf, spi_conf, sizeof(pwe_io_spi_config_t));
    memcpy(pwe_spi, &temp_conf, sizeof(pwe_io_spi_handle_t));
    pwe_spi->base.init = pwe_io_spi_init;
    pwe_spi->base.deinit = pwe_io_spi_deinit;
    pwe_spi->base.convert_buffer = pwe_io_spi_convert_buffer;
    pwe_spi->base.write = pwe_io_spi_write;
    pwe_spi->base.on_the_fly_send = NULL;
    pwe_spi->base.ensure_rst = pwe_io_spi_ensure_rst;
    pwe_spi->base.max_payload_length = buffer_size;
    *handle = &pwe_spi->base;
    return ESP_OK;
}

esp_err_t pwe_delete_spi_backend(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_spi_handle_t *pwe_spi = __containerof(handle, pwe_io_spi_handle_t, base);
    heap_caps_free(pwe_spi);
    return ESP_OK;
}
