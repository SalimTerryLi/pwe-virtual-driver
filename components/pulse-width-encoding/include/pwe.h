/*
 * SPDX-FileCopyrightText: SalimTerryLi <lhf2613@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/*
 * Pulse Width Encoding
 *  |    T0H  T0L   T1H  T1L          TxH  TxL     TRST    TxH
 *  |    +--+     +-----+  +-- ...... --+     |            +--
 *  |    |  |     |     |  |            |     |            |   ......
 *  |  --+  +-----+     +--+            +------------------+
 *  +------------------------------------------------------------------->
 *
 *  Currently mixing io driver and user level handler together
 */
typedef struct pwe_s pwe_t;

/**
* @brief PWE Type
*
*/
typedef pwe_t *pwe_handle_t;

/**
* @brief PWE generic configuration Type
*
*/
typedef struct {
    uint32_t T1H;       /*<! T1H, ns */
    uint32_t T1L;
    uint32_t T0H;
    uint32_t T0L;
    uint32_t TRST;
    uint32_t T1H_ACC;   /*<! T1H accept range */
    uint32_t T1L_ACC;   /*<! T1L accept range */
    uint32_t T0H_ACC;   /*<! T0H accept range */
    uint32_t T0L_ACC;   /*<! T0L accept range */
} pwe_config_t;

typedef esp_err_t (*pwe_iodriver_init)(pwe_handle_t handle);
typedef esp_err_t (*pwe_iodriver_on_the_fly_send)(pwe_handle_t handle, const void *data, uint32_t len);
typedef esp_err_t (*pwe_iodriver_convert_buffer)(pwe_handle_t handle, const void *data, uint32_t len, uint32_t *outgoing_buffer_len);
typedef esp_err_t (*pwe_iodriver_write)(pwe_handle_t handle, uint32_t len);
typedef esp_err_t (*pwe_iodriver_ensure_rst)(pwe_handle_t handle);
typedef esp_err_t (*pwe_iodriver_deinit)(pwe_handle_t handle);

/**
* @brief Declare of PWE handle Type
*
*/
struct pwe_s {
    pwe_iodriver_init init;
    pwe_iodriver_deinit deinit;
    pwe_iodriver_on_the_fly_send on_the_fly_send;
    pwe_iodriver_convert_buffer convert_buffer;
    pwe_iodriver_write write;
    pwe_iodriver_ensure_rst ensure_rst;
    uint32_t max_payload_length;
};

/**
 * @brief Init PWE driver
 *
 * @param handle: PWE handle
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_init(pwe_handle_t handle);

/**
 * @brief Deinit PWE driver
 *
 * @param handle: PWE handle
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_deinit(pwe_handle_t handle);

/**
* @brief Send n bits of data with pwe
 *
 * If PWE is created with zero length outgoing buffer then this API will try to work in streaming converting mode hence
 * reduce RAM usage but may slightly increase required computing power during sending progress. Furthermore if driver
 * doesn't support streaming converting then a runtime error is reported
*
* @param handle: PWE handle
* @param data: data to be sent
* @param len: length to be sent, in bits
* @return
*      ESP_OK
*/
esp_err_t pwe_send(pwe_handle_t handle, const void *data, uint32_t len);

/**
 * @brief Convert data and filling them to outgoing buffer(MSBit)
 *
 *         src[0]                 src[1]                 src[2]
 * [ 7 6 5 4 3 2 1 0 ] [ 15 14 13 12 11 10 9 8 ] [ 23 22 x x x x x x ]
 *                      ||
 * [ 7 6 5 4 3 2 1 0 15 14 13 12 11 10 9 8 23 22 ]
 *
 * @param handle: PWE handle
 * @param data: data to be converted and filled into outgoing buffer
 * @param len: length of data, in bits
 * @param outgoing_buffer_len: return length of raw data in outgoing buffer, to be used in pwe_io_write() later
 * @return
 *      ESP_OK
 */
esp_err_t pwe_io_convert_buffer(pwe_handle_t handle, const void *data, uint32_t len, uint32_t *outgoing_buffer_len);

/**
 * @brief Write out outgoing buffer
 *
 * @param handle: PWE handle
 * @param len: length of outgoing buffer to be sent, in bits
 *
 * @note len should be acquired from the offset value returned from pwe_io_fill_buffer()
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_io_write(pwe_handle_t handle, uint32_t len);

/**
 * @brief Delay a period defined by TRST
 *
 * @param handle: PWE handle
 *
 * @return
 *      ESP_OK
 */
esp_err_t pwe_ensure_rst(pwe_handle_t handle);

#ifdef __cplusplus
}
#endif
