/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "dshot.h"
#include "pwe_io_rmt.h"

static const char *TAG = "DSHOT";

#define DSHOT_THROTTLE_POSITION     5u
#define DSHOT_TELEMETRY_POSITION    4u
#define NIBBLES_SIZE                4u
#define DSHOT_NUMBER_OF_NIBBLES     3u

struct dshot_s {
    pwe_handle_t pwe;
    esp_timer_handle_t periodic_timer;
    uint32_t io_buffer_len;
    spinlock_t spinlock;
};

esp_err_t dshot_new_pwe_rmt(const pwe_config_t *pwe_conf, const rmt_config_t *rmt_conf, dshot_handle_t *hdl)
{
    ESP_RETURN_ON_FALSE(pwe_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL pwe_conf");
    ESP_RETURN_ON_FALSE(rmt_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL rmt_conf");
    esp_err_t ret = ESP_OK;
    pwe_handle_t pwe_handle = NULL;
    rmt_config_t rmtconf;
    memcpy(&rmtconf, rmt_conf, sizeof(rmt_config_t));
    rmtconf.clk_div = 4;
    ESP_RETURN_ON_ERROR(pwe_new_rmt_backend(pwe_conf, &rmtconf, 16, &pwe_handle), TAG, "Failed to create pwe driver");

    dshot_handle_t dshot_handle = malloc(sizeof(dshot_t));
    ESP_GOTO_ON_FALSE(hdl != NULL, ESP_ERR_NO_MEM, err_dshot_alloc, TAG, "Failed to allocate dshot_handle_t");

    dshot_handle->pwe = pwe_handle;
    ESP_GOTO_ON_ERROR(pwe_init(pwe_handle), err_pwe_init, TAG, "Failed to init pwe");

    spinlock_initialize(&dshot_handle->spinlock);

    *hdl = dshot_handle;
    return ESP_OK;

err_pwe_init:
    free(dshot_handle);
err_dshot_alloc:
    ESP_ERROR_CHECK(pwe_delete_rmt_backend(pwe_handle));
    return ret;
}

esp_err_t dshot_del_pwe_rmt(dshot_handle_t hdl)
{
    ESP_RETURN_ON_FALSE(hdl != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL dshot handle");
    ESP_RETURN_ON_ERROR(pwe_deinit(hdl->pwe), TAG, "Failed to deinit pwe");
    ESP_RETURN_ON_ERROR(pwe_delete_rmt_backend(hdl->pwe), TAG, "Failed delete pwe");
    free(hdl);
    return ESP_OK;
}

esp_err_t dshot_new_pwe_spi(const pwe_config_t *pwe_conf, const pwe_io_spi_config_t *spi_conf, dshot_handle_t *hdl)
{
    ESP_RETURN_ON_FALSE(pwe_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL pwe_conf");
    ESP_RETURN_ON_FALSE(spi_conf != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL rmt_conf");
    esp_err_t ret = ESP_OK;
    pwe_handle_t pwe_handle;
    ESP_RETURN_ON_ERROR(pwe_new_spi_backend(pwe_conf, spi_conf, 16, &pwe_handle), TAG, "Failed to create pwe driver");

    dshot_handle_t dshot_handle = malloc(sizeof(dshot_t));
    ESP_GOTO_ON_FALSE(hdl != NULL, ESP_ERR_NO_MEM, err_dshot_alloc, TAG, "Failed to allocate dshot_handle_t");

    dshot_handle->pwe = pwe_handle;
    ESP_GOTO_ON_ERROR(pwe_init(pwe_handle), err_pwe_init, TAG, "Failed to init pwe");

    spinlock_initialize(&dshot_handle->spinlock);

    *hdl = dshot_handle;
    return ESP_OK;

err_pwe_init:
    free(dshot_handle);
err_dshot_alloc:
    ESP_ERROR_CHECK(pwe_delete_rmt_backend(pwe_handle));
    return ret;
}

esp_err_t dshot_del_pwe_spi(dshot_handle_t hdl)
{
    ESP_RETURN_ON_FALSE(hdl != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL dshot handle");
    ESP_RETURN_ON_ERROR(pwe_deinit(hdl->pwe), TAG, "Failed to deinit pwe");
    ESP_RETURN_ON_ERROR(pwe_delete_spi_backend(hdl->pwe), TAG, "Failed delete pwe");
    free(hdl);
    return ESP_OK;
}

static void periodic_timer_callback(void *arg)
{
    dshot_handle_t hdl = (dshot_handle_t)arg;
    spinlock_acquire(&hdl->spinlock, SPINLOCK_WAIT_FOREVER);
    pwe_io_write(hdl->pwe, hdl->io_buffer_len);
    spinlock_release(&hdl->spinlock);
}

esp_err_t dshot_start(dshot_handle_t hdl, uint32_t interval_us)
{
    ESP_RETURN_ON_FALSE(hdl != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL dshot handle");
    ESP_RETURN_ON_ERROR(dshot_update(hdl, 0, false), TAG, "Failed to update initial Dshot message");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "Dshot",
        .arg = hdl,
        .skip_unhandled_events = true,
        .dispatch_method = ESP_TIMER_TASK,
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&periodic_timer_args, &hdl->periodic_timer), TAG, "Faild to create esp_timer");
    return esp_timer_start_periodic(hdl->periodic_timer, interval_us);
}

esp_err_t dshot_stop(dshot_handle_t hdl)
{
    ESP_RETURN_ON_FALSE(hdl != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL dshot handle");
    return esp_timer_stop(hdl->periodic_timer);
}

esp_err_t dshot_update(dshot_handle_t hdl, uint16_t thrust, bool request_telemetry)
{
    ESP_RETURN_ON_FALSE(hdl != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL dshot handle");
    ESP_RETURN_ON_FALSE(thrust <= 2000, ESP_ERR_INVALID_ARG, TAG, "thrust out of range");

    thrust = thrust == 0 ? 0 : thrust + 47;

    /*
     * Refer to https://github.com/PX4/PX4-Autopilot/blob/master/platforms/nuttx/src/px4/stm/stm32_common/dshot/dshot.c
     */
    uint16_t packet = 0;
    uint16_t checksum = 0;
    packet |= thrust << DSHOT_THROTTLE_POSITION;
    packet |= ((uint16_t)request_telemetry & 0x01) << DSHOT_TELEMETRY_POSITION;

    uint16_t csum_data = packet;
    /* XOR checksum calculation */
    csum_data >>= NIBBLES_SIZE;

    for (unsigned i = 0; i < DSHOT_NUMBER_OF_NIBBLES; i++) {
        checksum ^= (csum_data & 0x0F); // XOR data by nibbles
        csum_data >>= NIBBLES_SIZE;
    }

    packet |= (checksum & 0x0F);

    // endian convert
    uint16_t out_buffer = 0x00;
    out_buffer |= packet >> 8;
    out_buffer |= packet << 8;

    esp_err_t ret = ESP_OK;
    spinlock_acquire(&hdl->spinlock, SPINLOCK_WAIT_FOREVER);
    ret = pwe_io_convert_buffer(hdl->pwe, (uint8_t *)&out_buffer, 16, &hdl->io_buffer_len);
    spinlock_release(&hdl->spinlock);
    return ret;
}
