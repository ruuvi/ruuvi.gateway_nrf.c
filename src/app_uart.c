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
#include "app_ble.h"
#include "app_uart.h"
#include "ruuvi_boards.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_endpoint_ca_uart.h"
#include "ruuvi_interface_communication.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_communication_uart.h"
#include "ruuvi_interface_log.h"
#include "ruuvi_interface_scheduler.h"

#include <string.h>
#include <stdio.h>

static inline void LOG (const char * msg)
{
    ri_log (RI_LOG_LEVEL_INFO, msg);
}

static inline void LOGD (const char * msg)
{
    ri_log (RI_LOG_LEVEL_DEBUG, msg);
}

static ri_comm_channel_t m_uart; //!< UART communication interface.

/** @brief convert baudrate from board definition for driver. */
static ri_uart_baudrate_t rb_to_ri_baud (const uint32_t rb_baud)
{
    ri_uart_baudrate_t baud;

    switch (rb_baud)
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


void uart_handler (void * p_event_data, uint16_t event_size)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_comm_message_t msg = {0};
    msg.data_length = sizeof (msg.data);
    err_code |= m_uart.read (&msg);
    // Handle data in &msg.data. Note that data might be fragmented,
    // the payload might contain `\n` and message has to be assembled from
    // several fragments.
    LOG ("RECEIVED: ");
    ri_log_hex (RI_LOG_LEVEL_INFO,
                msg.data,
                msg.data_length);
    LOG ("\n");
    err_code |= app_ble_scan_start();
}

static rd_status_t uart_isr (ri_comm_evt_t evt,
                             void * p_data, size_t data_len)
{
    switch (evt)
    {
        case RI_COMM_CONNECTED:
            // Will never trigger on UART
            break;

        case RI_COMM_DISCONNECTED:
            // Will never trigger on UART
            break;

        case RI_COMM_SENT:
            LOGD ("Data sent\r\n");
            break;

        case RI_COMM_RECEIVED:
            LOG ("Data received\r\n");
            ri_scheduler_event_put (NULL, 0, &uart_handler);
            break;

        default:
            break;
    }

    return RD_SUCCESS;
}

static void setup_uart_init (ri_uart_init_t * const p_init)
{
    // Ruuvi board pin definitions are compatible with
    // Ruuvi driver pin definitions.
#if APP_USBUART_ENABLED
    p_init->hwfc_enabled = false;
    p_init->parity_enabled = RB_UART_USB_PARITY_ENABLED;
    p_init->cts = RI_GPIO_ID_UNUSED;
    p_init->rts = RI_GPIO_ID_UNUSED;
    p_init->tx = RB_UART_USB_TX_PIN;
    p_init->rx = RB_UART_USB_RX_PIN;
    p_init->baud = rb_to_ri_baud (RB_UART_BAUDRATE);
#else
    p_init->hwfc_enabled = RB_HWFC_ENABLED;
    p_init->parity_enabled = RB_PARITY_ENABLED;
    p_init->cts  = RB_UART_CTS_PIN;
    p_init->rts  = RB_UART_RTS_PIN;
    p_init->tx   = RB_UART_TX_PIN;
    p_init->rx   = RB_UART_RX_PIN;
    p_init->baud = rb_to_ri_baud (RB_UART_BAUDRATE);
#endif
}

rd_status_t app_uart_init (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_uart_init_t config = { 0 };
    setup_uart_init (&config);
    err_code |= ri_uart_init (&m_uart);

    if (RD_SUCCESS == err_code)
    {
        err_code |= ri_uart_config (&config);
        m_uart.on_evt = &uart_isr;
    }

    return err_code;
}

rd_status_t app_uart_send_broadcast (const ri_adv_scan_t * const scan)
{
    re_ca_uart_payload_t adv = {0};
    ri_comm_message_t msg = {0};
    rd_status_t err_code = RD_SUCCESS;
    re_status_t re_code = RE_SUCCESS;

    if (NULL == scan)
    {
        err_code |= RD_ERROR_NULL;
    }
    else if (RE_CA_UART_ADV_BYTES >= scan->data_len)
    {
        memcpy (adv.params.adv.mac, scan->addr, sizeof (adv.params.adv.mac));
        memcpy (adv.params.adv.adv, scan->data, scan->data_len);
        adv.params.adv.rssi_db = scan->rssi;
        adv.params.adv.adv_len = scan->data_len;
        adv.cmd = RE_CA_UART_ADV_RPRT;
        msg.data_length = sizeof (msg);
        re_code = re_ca_uart_encode (msg.data, &msg.data_length, &adv);
        msg.repeat_count = 1;

        if (RE_SUCCESS == re_code)
        {
            err_code |= m_uart.send (&msg);
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
