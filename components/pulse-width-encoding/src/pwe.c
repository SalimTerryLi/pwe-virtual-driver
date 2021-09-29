/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pwe.h"
#include "esp_check.h"

static const char *TAG = "PWE";

esp_err_t pwe_init(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ESP_RETURN_ON_FALSE(handle->init != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL init implementation");
    return handle->init(handle);
}

esp_err_t pwe_deinit(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ESP_RETURN_ON_FALSE(handle->deinit != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL deinit implementation");
    return handle->deinit(handle);
}

esp_err_t pwe_send(pwe_handle_t handle, const void *data, uint32_t len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    uint32_t outgoing_buffer_size = 0;
    if (handle->max_payload_length == 0) {
        if (handle->on_the_fly_send != NULL) {
            return handle->on_the_fly_send(handle, data, len);
        } else {
            ESP_LOGE(TAG, "on_the_fly_send() not supported by driver");
            return ESP_ERR_INVALID_STATE;
        }
    } else {
        ESP_RETURN_ON_FALSE(len <= handle->max_payload_length, ESP_ERR_INVALID_ARG, TAG, "Insufficient buffer size");
        ESP_RETURN_ON_ERROR(pwe_io_convert_buffer(handle, data, len, &outgoing_buffer_size), TAG, "Failed to fill outgoing buffer");
        ESP_RETURN_ON_ERROR(pwe_io_write(handle, outgoing_buffer_size), TAG, "Failed to write out data");
    }
    return ESP_OK;
}

esp_err_t pwe_io_convert_buffer(pwe_handle_t handle, const void *data, uint32_t len, uint32_t *outgoing_buffer_len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ESP_RETURN_ON_FALSE(handle->convert_buffer != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL cunvert_buffer implementation");
    ESP_RETURN_ON_FALSE(len <= handle->max_payload_length, ESP_ERR_INVALID_ARG, TAG, "Insufficient buffer size");
    return handle->convert_buffer(handle, data, len, outgoing_buffer_len);
}

esp_err_t pwe_io_write(pwe_handle_t handle, uint32_t len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL handle");
    ESP_RETURN_ON_FALSE(handle->write != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL io_write implementation");
    return handle->write(handle, len);
}

esp_err_t pwe_ensure_rst(pwe_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle->ensure_rst != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL ensure_rst implementation");
    return handle->ensure_rst(handle);
}
