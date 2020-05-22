#ifndef UART_H
#define UART_H

/** 
 * @defgroup APP_UART Application UART functionality.
 * @{ 
 */
/**
 *  @file app_ble.h
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-22
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application UART control, sending data and handling received commands.
 */



#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication_ble_advertising.h"

#define UART_BUFFER_MAX_LEN 256
#define UART_OUT_MAX 200

#define STX 0x02
#define ETX 0x03
#define CMD1 1
#define CMD2 2
#define CMD1_LEN 3
#define CMD2_LEN 1

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
 * @retval RD_SUCCESS On successful init.
 * @retval RD_ERROR_INVALID_STATE If UART is already initialized.
 */
void app_uart_send_broadcast (const ri_adv_scan_t * const scan);

/** @} */
#endif
