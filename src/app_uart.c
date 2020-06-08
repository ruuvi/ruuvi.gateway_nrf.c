/** 
 * @addtogroup APP_UART 
 * @{ 
 */
/** 
 *  @file app_uart.c
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-29
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application UART control, processing incoming data and sending data out.
 */
#include "app_config.h"
#include "app_uart.h"
#include "ruuvi_boards.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_endpoint_ca_uart.h"
#include "ruuvi_interface_communication.h" 
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_communication_uart.h" 

#include <string.h>
#include <stdio.h>
#define INIT_STRING "Ave Mundi!\n" // Hello world to send via UART.

static ri_comm_channel_t m_uart; // UART communication interface.

/** @brief convert baudrate from board definition for driver. */
static ri_uart_baudrate_t rb_to_ri_baud(const uint32_t rb_baud)
{
    ri_uart_baudrate_t baud;
    switch(rb_baud)
    {
        case RB_UART_BAUDRATE_9600:
            baud = RI_UART_BAUD_9600;
            break;

        case RB_UART_BAUDRATE_115200:
            baud = RI_UART_BAUD_115200;
            break;

        default:
            baud = RI_UART_BAUD_115200;
            break;
    }
    return baud;
}

static void setup_uart_init(ri_uart_init_t* const p_init)
{
    // Ruuvi board pin definitions are compatible with 
    // Ruuvi driver pin definitions.
#if APP_USBUART_ENABLED
    p_init->hwfc_enabled = RB_UART_USB_HWFC_ENABLED;   
    p_init->parity_enabled = RB_UART_USB_PARITY_ENABLED; 
    p_init->cts = RB_UART_USB_CTS_PIN;
    p_init->rts = RB_UART_USB_RTS_PIN;
    p_init->tx = RB_UART_USB_TX_PIN;
    p_init->rx = RB_UART_USB_RX_PIN;
    p_init->baud = rb_to_ri_baud(RB_UART_BAUDRATE); 
#else
    p_init->hwfc_enabled = RB_HWFC_ENABLED;  
    p_init->parity_enabled = RB_PARITY_ENABLED;
    p_init->cts  = RB_UART_CTS_PIN;
    p_init->rts  = RB_UART_RTS_PIN;
    p_init->tx   = RB_UART_TX_PIN;
    p_init->rx   = RB_UART_RX_PIN;
    p_init->baud = rb_to_ri_baud(RB_UART_BAUDRATE);
#endif
}

rd_status_t app_uart_init (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_uart_init_t config = { 0 };
    ri_comm_message_t msg = { 0 };
    setup_uart_init(&config);
    err_code |= ri_uart_init(&m_uart);
    err_code |= ri_uart_config(&config);
    msg.repeat_count = 1;
    snprintf((char*)msg.data, sizeof(msg.data), INIT_STRING);
    msg.data_length = strlen(INIT_STRING);
    err_code |= m_uart.send(&msg);
}

rd_status_t app_uart_send_broadcast (const ri_adv_scan_t * const scan)
{
    re_ca_uart_payload_t adv = {0};
    ri_comm_message_t msg = {0};
    rd_status_t err_code = RD_SUCCESS;
    re_status_t re_code = RE_SUCCESS;

    if(RE_CA_UART_ADV_BYTES >= scan->data_len)
    {
        memcpy(adv.params.adv.mac, scan->addr, sizeof(adv.params.adv.mac));
        memcpy(adv.params.adv.adv, scan->data, scan->data_len);
        adv.params.adv.rssi_db = scan->rssi;
        adv.params.adv.adv_len = scan->data_len;
        adv.cmd = RE_CA_UART_ADV_RPRT;
        msg.data_length = sizeof(msg);
        re_code = re_ca_uart_encode(msg.data, &msg.data_length, &adv);
        msg.repeat_count = 1; 
        if(RE_SUCCESS == re_code)
        {
            err_code |= m_uart.send(&msg);
        }
        else
        {
            err_code |= RD_ERROR_INVALID_DATA;
        }
    }
    else
    {
        err_code |= RD_ERROR_DATA_SIZE;
    }
    return err_code;
}

/** @} */
