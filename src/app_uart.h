#ifndef UART_H
#define UART_H

/**
 * @defgroup APP_UART Application UART functionality.
 * @{
 */
/**
 *  @file app_ble.h
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-28
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application UART control, sending data and handling received commands.
 */



#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication_ble_advertising.h"

#ifdef CEEDLING
// Assist function for unit tests.
bool app_uart_ringbuffer_lock_dummy (volatile uint32_t * const flag, bool lock);
void app_uart_parser (void * p_data, uint16_t data_len);

// Expose callback to Ceedling
rd_status_t app_uart_apply_config (void * v_uart_payload);
rd_status_t app_uart_isr (ri_comm_evt_t evt,
                          void * p_data, size_t data_len);

#endif

/**
 * @brief Initialize UART peripheral with values read from ruuvi_boards.h.
 *
 * After initialization UART peripheral is active and ready to handle incoming data
 * and send data out.
 *
 * @retval RD_SUCCESS On successful init.
 * @retval RD_ERROR_INVALID_STATE If UART is already initialized.
 */
rd_status_t app_uart_init (void);

/**
 * @brief Send a scanned BLE broadcast through UART.
 *
 * The format is defined by ruuvi.endpoints.c/
 *
 * @param[in] scan Scan result from BLE advertisement module.
 * @retval RD_SUCCESS If encoding and queuing data to UART was successful.
 * @retval RD_ERROR_NULL If scan was NULL.
 * @retval RD_ERROR_INVALID_DATA If scan cannot be encoded for any reason.
 * @retval RD_ERROR_DATA_SIZE If scan had larger advertisement size than allowed by
 *                            encoding module.
 */
rd_status_t app_uart_send_broadcast (const ri_adv_scan_t * const scan);

/** @} */
#endif
