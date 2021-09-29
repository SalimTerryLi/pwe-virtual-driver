/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "pwe.h"
#include "dshot.h"

static const char *TAG = "example";

#if CONFIG_EXAMPLE_DSHOT_MODE_150
#define DSHOT_CONFIG PWE_DSHOT150_CONFIG
#elif CONFIG_EXAMPLE_DSHOT_MODE_300
#define DSHOT_CONFIG PWE_DSHOT300_CONFIG
#elif CONFIG_EXAMPLE_DSHOT_MODE_600
#define DSHOT_CONFIG PWE_DSHOT600_CONFIG
#elif CONFIG_EXAMPLE_DSHOT_MODE_1200
#define DSHOT_CONFIG PWE_DSHOT1200_CONFIG
#endif

void app_main(void)
{
    pwe_config_t pwe_conf = DSHOT_CONFIG;
    rmt_config_t rmt_conf = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_DSHOT_TX_GPIO, RMT_CHANNEL_0);
    dshot_handle_t dshot_hdl = NULL;
    ESP_ERROR_CHECK(dshot_new_pwe_rmt(&pwe_conf, &rmt_conf, &dshot_hdl));

    if (dshot_hdl == NULL) {
        ESP_LOGE(TAG, "Failed to create pwe driver");
    }
    ESP_ERROR_CHECK(dshot_start(dshot_hdl, 1000));
    ESP_LOGI(TAG, "Dshot start");
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "spin motor to 1/10 throttle");
    ESP_ERROR_CHECK(dshot_update(dshot_hdl, 200, false));
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "spin motor to 1/5 throttle");
    ESP_ERROR_CHECK(dshot_update(dshot_hdl, 400, false));
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "stop motor");
    ESP_ERROR_CHECK(dshot_update(dshot_hdl, 0, false));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(dshot_stop(dshot_hdl));
    ESP_ERROR_CHECK(dshot_del_pwe_rmt(dshot_hdl));
}
