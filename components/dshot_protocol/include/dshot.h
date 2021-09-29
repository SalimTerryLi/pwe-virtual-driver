/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "driver/rmt.h"
#include "pwe_io_spi.h"
#include "esp_err.h"
#include "pwe.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default configuration for Dshot protocol
 *
 */
#define PWE_DSHOT150_CONFIG         \
    {                               \
        .T1H = 5000,                \
        .T1L = 1666,                \
        .T0H = 2500,                \
        .T0L = 4167,                \
        .T1H_ACC = 800,             \
        .T1L_ACC = 800,             \
        .T0H_ACC = 800,             \
        .T0L_ACC = 800,             \
        .TRST = 13333,              \
    }

#define PWE_DSHOT300_CONFIG         \
    {                               \
        .T1H = 2500,                \
        .T1L = 833,                 \
        .T0H = 1250,                \
        .T0L = 2083,                \
        .T1H_ACC = 400,             \
        .T1L_ACC = 400,             \
        .T0H_ACC = 400,             \
        .T0L_ACC = 400,             \
        .TRST = 6666,               \
    }

#define PWE_DSHOT600_CONFIG         \
    {                               \
        .T1H = 1250,                \
        .T1L = 416,                 \
        .T0H = 625,                 \
        .T0L = 1041,                \
        .T1H_ACC = 200,             \
        .T1L_ACC = 200,             \
        .T0H_ACC = 200,             \
        .T0L_ACC = 200,             \
        .TRST = 3333,               \
    }

#define PWE_DSHOT1200_CONFIG        \
    {                               \
        .T1H = 625,                 \
        .T1L = 208,                 \
        .T0H = 313,                 \
        .T0L = 520,                 \
        .T1H_ACC = 100,             \
        .T1L_ACC = 100,             \
        .T0H_ACC = 100,             \
        .T0L_ACC = 100,             \
        .TRST = 1666,               \
    }

typedef enum {
    DShot_cmd_motor_stop = 0,
    DShot_cmd_beacon1,
    DShot_cmd_beacon2,
    DShot_cmd_beacon3,
    DShot_cmd_beacon4,
    DShot_cmd_beacon5,
    DShot_cmd_esc_info, // V2 includes settings
    DShot_cmd_spin_direction_1,
    DShot_cmd_spin_direction_2,
    DShot_cmd_3d_mode_off,
    DShot_cmd_3d_mode_on,
    DShot_cmd_settings_request, // Currently not implemented
    DShot_cmd_save_settings,
    DShot_cmd_spin_direction_normal = 20,
    DShot_cmd_spin_direction_reversed = 21,
    DShot_cmd_led0_on, // BLHeli32 only
    DShot_cmd_led1_on, // BLHeli32 only
    DShot_cmd_led2_on, // BLHeli32 only
    DShot_cmd_led3_on, // BLHeli32 only
    DShot_cmd_led0_off, // BLHeli32 only
    DShot_cmd_led1_off, // BLHeli32 only
    DShot_cmd_led2_off, // BLHeli32 only
    DShot_cmd_led4_off, // BLHeli32 only
    DShot_cmd_audio_stream_mode_on_off = 30, // KISS audio Stream mode on/off
    DShot_cmd_silent_mode_on_off = 31, // KISS silent Mode on/off
    DShot_cmd_signal_line_telemeetry_disable = 32,
    DShot_cmd_signal_line_continuous_erpm_telemetry = 33,
    DShot_cmd_MAX = 47,
    DShot_cmd_MIN_throttle = 48
                             // >47 are throttle values
} dshot_command_t;

typedef struct dshot_s dshot_t;

typedef dshot_t *dshot_handle_t;

/**
 * @brief Create Dshot instance from given pwe configuration
 *
 * @param pwe: pwe handle
 * @param rmt_conf: rmt config
 * @param hdl: created dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_new_pwe_rmt(const pwe_config_t *pwe_conf, const rmt_config_t *rmt_conf, dshot_handle_t *hdl);

/**
 * @brief Delete Dshot instance created from pwe driver
 *
 * @param hdl: dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_del_pwe_rmt(dshot_handle_t hdl);

/**
 * @brief Create Dshot instance from given pwe configuration
 *
 * @param pwe: pwe handle
 * @param spi_conf: spi config
 * @param hdl: created dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_new_pwe_spi(const pwe_config_t *pwe_conf, const pwe_io_spi_config_t *spi_conf, dshot_handle_t *hdl);

/**
 * @brief Delete Dshot instance created from pwe driver
 *
 * @param hdl: dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_del_pwe_spi(dshot_handle_t hdl);

/**
 * @brief Start Dshot output
 *
 * @param hdl: dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_start(dshot_handle_t hdl, uint32_t interval_us);

/**
 * @brief Stop Dshot output
 *
 * @param hdl: dshot instance
 * @return
 *      ESP_OK
 */
esp_err_t dshot_stop(dshot_handle_t hdl);

/**
 * @brief Send control message to ESC
 *
 * @param pwe: pwe handle
 * @param thrust: 0: disarming, 1~2000 for throttle
 * @param request_telemetry: whether telemetry is requested or not
 * @return
 *      ESP_OK
 */
esp_err_t dshot_update(dshot_handle_t hdl, uint16_t thrust, bool request_telemetry);

#ifdef __cplusplus
}
#endif
