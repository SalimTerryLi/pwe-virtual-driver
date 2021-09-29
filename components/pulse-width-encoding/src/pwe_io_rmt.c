/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_heap_caps.h"
#include "pwe_io_rmt.h"
#include "esp_check.h"

static const char *TAG = "PWE_IO_RMT";

#define UINTROUNDDIV(divd, divor) ( ((divd) + ((divor) / 2)) / (divor) )
#define UINTCEILDIV(divd, divor) ( ((divd) + (divor) - 1) / (divor) )

static inline uint32_t calc_aligned_error(uint32_t divided, uint32_t divisor)
{
    return abs((int)(divided - UINTROUNDDIV(divided, divisor) * divisor));
}

typedef struct {
    struct pwe_s base;
    rmt_config_t rmt_conf;
    uint32_t trst;
    uint16_t t1h;
    uint16_t t1l;
    uint16_t t0h;
    uint16_t t0l;
    rmt_item32_t buffer[0];
    size_t _total_bits_to_send; // workaround: rmt translator only accept byte
} pwe_io_rmt_handle_t;

/**
 * @brief Convert raw bit data in u8[] to RMT format.
 *
 *        src[0]                 src[1]
 * [ 7 6 5 4 3 2 1 0 ] [ 15 14 13 12 11 10 9 8 ]
 *                      ||
 * RMT seq: [ 7 6 5 4 3 2 1 0 15 14 13 12 11 10 9 8 ]
 *
 * src = {0b00000001, 0b00000010} on ESP32(MSBit) will generate RMT sequence(MSBit) {0000000100000010}
 *
 * @param[in] src: source data, to converted to RMT format
 * @param[in] dest: place where to store the convert result
 * @param[in] src_size: size of source data
 * @param[in] wanted_num: number of RMT items that want to get
 * @param[out] translated_size: number of source data that got converted
 * @param[out] item_num: number of RMT items which are converted from source data
 */
static void IRAM_ATTR pwe_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
                                      size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    /*
     * Implementation details:
     * - Must finish whole byte translate and must not leave a half-translated byte here, so that in the next call we can start from the first bit of src
     * - Our stop condition is bit not byte so that a workaround (_total_bits_to_send) is here to let us compare with item_num.
     */
    pwe_io_rmt_handle_t *pwe_rmt;
    rmt_translator_get_context(item_num, (void **) &pwe_rmt);
    if (src == NULL || dest == NULL || pwe_rmt == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {{{ pwe_rmt->t0h, 1, pwe_rmt->t0l, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ pwe_rmt->t1h, 1, pwe_rmt->t1l, 0 }}}; //Logical 1
    size_t translated_byte_num = 0;
    size_t translated_bit_num = 0;
    const size_t bytes_can_be_translated = wanted_num / 8; // ensure byte width align
    /* calculate remain bits by: total_bits - (bytes_that_total_bits_consume - remain_bytes) * 8 */
    size_t bits_remain_in_input = pwe_rmt->_total_bits_to_send - (UINTCEILDIV(pwe_rmt->_total_bits_to_send, 8) - src_size) * 8;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (translated_byte_num < bytes_can_be_translated) { // not all bits are translated but can not do more
        for (int i = 0; i < 8; i++) {
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bit1.val;
            } else {
                pdest->val =  bit0.val;
            }
            translated_bit_num++;
            pdest++;
            if (--bits_remain_in_input == 0) {  // the only termination point that all bits are translated
                break;
            }
        }
        translated_byte_num++;
        psrc++;
        if (bits_remain_in_input == 0) {
            break;
        }
    }
    *translated_size = translated_byte_num;
    *item_num = translated_bit_num;
}

static esp_err_t pwe_io_rmt_init(pwe_handle_t handle)
{
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    ESP_RETURN_ON_ERROR(rmt_config(&pwe_rmt->rmt_conf), TAG, "Failed to configure RMT");
    ESP_RETURN_ON_ERROR(rmt_driver_install(pwe_rmt->rmt_conf.channel, 0, 0), TAG, "Failed to install RMT driver");
    ESP_RETURN_ON_ERROR(rmt_translator_init(pwe_rmt->rmt_conf.channel, pwe_rmt_adapter), TAG, "Failed to set translator");
    ESP_RETURN_ON_ERROR(rmt_translator_set_context(pwe_rmt->rmt_conf.channel, pwe_rmt), TAG, "Failed to set RMT context");
    return ESP_OK;
}

static esp_err_t pwe_io_rmt_deinit(pwe_handle_t handle)
{
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    ESP_RETURN_ON_ERROR(rmt_driver_uninstall(pwe_rmt->rmt_conf.channel), TAG, "Failed to uninstall RMT driver");
    return ESP_OK;
}

static esp_err_t pwe_io_rmt_convert_buffer(pwe_handle_t handle, const void *data, uint32_t len, uint32_t *outgoing_buffer_len)
{
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    ESP_RETURN_ON_FALSE(pwe_rmt->base.max_payload_length >= len, ESP_ERR_INVALID_ARG, TAG, "len too big");
    const rmt_item32_t bit0 = {{{ pwe_rmt->t0h, 1, pwe_rmt->t0l, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ pwe_rmt->t1h, 1, pwe_rmt->t1l, 0 }}}; //Logical 1
    uint32_t bits_src_proceeded = 0;
    while (bits_src_proceeded < len) {
        uint32_t byte_offset = bits_src_proceeded / 8;
        uint8_t bit_offset = (7 - bits_src_proceeded % 8);
        if ((((uint8_t *)data)[byte_offset]) & (1 << bit_offset)) {
            pwe_rmt->buffer[bits_src_proceeded].val = bit1.val;
        } else {
            pwe_rmt->buffer[bits_src_proceeded].val = bit0.val;
        }
        ++bits_src_proceeded;
    }
    *outgoing_buffer_len = bits_src_proceeded;
    return ESP_OK;
}

static esp_err_t pwe_io_rmt_write(pwe_handle_t handle, uint32_t len)
{
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    ESP_RETURN_ON_ERROR(rmt_write_items(pwe_rmt->rmt_conf.channel, pwe_rmt->buffer, len, true), TAG, "Failed to write items");
    return ESP_OK;
}

static esp_err_t pwe_io_rmt_on_the_fly_send(pwe_handle_t handle, const void *data, uint32_t len)
{
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    pwe_rmt->_total_bits_to_send = len; // workaround
    rmt_write_sample(pwe_rmt->rmt_conf.channel, data, UINTCEILDIV(len, 8), true);
    return ESP_OK;
}

static esp_err_t pwe_io_spi_ensure_rst(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    // first make sure output is low
    rmt_item32_t rmt_item = {
        .duration0 = 1,
        .duration1 = 1,
        .level0 = 0,
        .level1 = 0,
    };
    rmt_write_items(pwe_rmt->rmt_conf.channel, &rmt_item, 1, true);
    // then delay
    esp_rom_delay_us(pwe_rmt->trst);
    return ESP_OK;
}

esp_err_t pwe_new_rmt_backend(const pwe_config_t *config, const rmt_config_t *rmt_conf, uint32_t buffer_size, pwe_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "null config");
    ESP_RETURN_ON_FALSE(rmt_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "null config");

    uint32_t RMT_BASE_CLK_HZ = APB_CLK_FREQ / rmt_conf->clk_div;
    uint32_t ITEM_MIN_STEP_NS = 1000000000 / RMT_BASE_CLK_HZ;
    uint32_t ITEM_MAX_PERIOD_NS = ITEM_MIN_STEP_NS * 0x7fff;
    // check if pulse can be presented with current clk
    ESP_RETURN_ON_FALSE(config->T1H <= ITEM_MAX_PERIOD_NS &&
                        config->T1L <= ITEM_MAX_PERIOD_NS &&
                        config->T0H <= ITEM_MAX_PERIOD_NS &&
                        config->T0L <= ITEM_MAX_PERIOD_NS,
                        ESP_ERR_INVALID_ARG, TAG, "TxH/TxL upper overflow: suggest increasing rmt clk_div");
    ESP_RETURN_ON_FALSE(calc_aligned_error(config->T1H, ITEM_MIN_STEP_NS) < config->T1H_ACC &&
                        calc_aligned_error(config->T1L, ITEM_MIN_STEP_NS) < config->T1L_ACC &&
                        calc_aligned_error(config->T0H, ITEM_MIN_STEP_NS) < config->T0H_ACC &&
                        calc_aligned_error(config->T0L, ITEM_MIN_STEP_NS) < config->T0L_ACC,
                        ESP_ERR_INVALID_ARG, TAG, "TxH/TxL bad resolution: suggest decreasing rmt clk_div");

    pwe_io_rmt_handle_t *pwe_rmt = malloc(sizeof(pwe_io_rmt_handle_t) + buffer_size * sizeof(rmt_item32_t));
    ESP_RETURN_ON_FALSE(pwe_rmt != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate pwe_io_rmt_handle_t");
    // convert from ns to us
    pwe_rmt->trst = config->TRST / 1000;
    pwe_rmt->trst = pwe_rmt->trst == 0 ? 1 : pwe_rmt->trst;
    // fill the calculated TxX
    pwe_rmt->t1h = UINTROUNDDIV(config->T1H, ITEM_MIN_STEP_NS);
    pwe_rmt->t1l = UINTROUNDDIV(config->T1L, ITEM_MIN_STEP_NS);
    pwe_rmt->t0h = UINTROUNDDIV(config->T0H, ITEM_MIN_STEP_NS);
    pwe_rmt->t0l = UINTROUNDDIV(config->T0L, ITEM_MIN_STEP_NS);

    memcpy(&pwe_rmt->rmt_conf, rmt_conf, sizeof(rmt_config_t));

    pwe_rmt->base.init = pwe_io_rmt_init;
    pwe_rmt->base.deinit = pwe_io_rmt_deinit;
    pwe_rmt->base.convert_buffer = pwe_io_rmt_convert_buffer;
    pwe_rmt->base.write = pwe_io_rmt_write;
    pwe_rmt->base.on_the_fly_send = pwe_io_rmt_on_the_fly_send;
    pwe_rmt->base.ensure_rst = pwe_io_spi_ensure_rst;
    pwe_rmt->base.max_payload_length = buffer_size;
    *handle = &pwe_rmt->base;
    return ESP_OK;
}

esp_err_t pwe_delete_rmt_backend(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "null handle");
    pwe_io_rmt_handle_t *pwe_rmt = __containerof(handle, pwe_io_rmt_handle_t, base);
    free(pwe_rmt);
    return ESP_OK;
}
