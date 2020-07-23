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
#include "ruuvi_interface_watchdog.h"
#include "ruuvi_interface_scheduler.h"
#include "ruuvi_interface_communication_uart.h"

#include <string.h>
#include <stdio.h>

static ri_comm_channel_t m_uart; //!< UART communication interface.

void app_uart_parser (void * p_data, uint16_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_comm_message_t msg = {0};
    re_ca_uart_payload_t uart_payload = {0};
    err_code = re_ca_uart_decode (p_data, &uart_payload);

    if (RD_SUCCESS == err_code)
    {
        uart_payload.params.bool_param.state = RE_CA_ACK_OK;
    }
    else
    {
        uart_payload.params.bool_param.state = RE_CA_ACK_ERROR;
    }

    //TODO!
    uart_payload.cmd = RE_CA_UART_ACK;
    msg.data_length = sizeof (msg);
    err_code = re_ca_uart_encode (msg.data, &msg.data_length, &uart_payload);
    msg.repeat_count = 1;

    if (RE_SUCCESS == err_code)
    {
        err_code |= m_uart.send (&msg);
    }
    else
    {
        err_code |= RD_ERROR_INVALID_DATA;
    }

    ri_watchdog_feed();
}

rd_status_t app_uart_isr (ri_comm_evt_t evt,
                          void * p_data, size_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;

    switch (evt)
    {
        case RI_COMM_SENT:
            break;

        case RI_COMM_RECEIVED:
            err_code |= ri_scheduler_event_put (p_data, (uint16_t) data_len, app_uart_parser);
            break;

        default:
            break;
    }

    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);
    return err_code;
}

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

static void setup_uart_init (ri_uart_init_t * const p_init)
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
        m_uart.on_evt = app_uart_isr;
        err_code |= ri_uart_config (&config);
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
