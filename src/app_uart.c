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
#include <string.h>
#include "app_ble.h"
#include "main.h"
#include "ruuvi_boards.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_endpoint_ca_uart.h"
#include "ruuvi_interface_communication.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_watchdog.h"
#include "ruuvi_interface_scheduler.h"
#include "ruuvi_interface_communication_uart.h"
#include "ruuvi_library_ringbuffer.h"
#include "ruuvi_interface_yield.h"
#include "ruuvi_task_led.h"
#ifndef CEEDLING
#include "nrf_log.h"
#else
#define NRF_LOG_INFO(fmt, ...)
#define NRF_LOG_ERROR(fmt, ...)
#define NRF_LOG_HEXDUMP_INFO(data, len)
#endif

#define APP_UART_RING_BUFFER_MAX_LEN     (128U) //!< Ring buffer len       
#define APP_UART_RING_DEQ_BUFFER_MAX_LEN (APP_UART_RING_BUFFER_MAX_LEN >>1) //!< Decode buffer len

/*!
 * @brief UART response type enum
 */
typedef enum
{
    APP_UART_RESP_TYPE_NONE = 0,  //!< No response
    APP_UART_RESP_TYPE_ACK,       //!< Ack response
    APP_UART_RESP_TYPE_DEVICE_ID, //!< Device ID response
} app_uart_resp_type_e;

#ifndef CEEDLING
static bool app_uart_ringbuffer_lock_dummy (volatile uint32_t * const flag, bool lock);
#endif

static ri_comm_channel_t m_uart; //!< UART communication interface.
static uint8_t buffer_data[APP_UART_RING_BUFFER_MAX_LEN] = {0};
static bool buffer_wlock = false;
static bool buffer_rlock = false;

static bool g_flag_uart_tx_in_progress;
static app_uart_resp_type_e g_resp_type;
static re_ca_uart_cmd_t g_resp_ack_cmd;
static bool g_resp_ack_state;

#ifndef CEEDLING
static
#endif
volatile bool m_uart_ack = false;

static rl_ringbuffer_t m_uart_ring_buffer =
    (rl_ringbuffer_t)
{
    .head = 0,
    .tail = 0,
    .block_size = sizeof (uint8_t),
    .storage_size = sizeof (buffer_data),
    .index_mask = (sizeof (buffer_data) / sizeof (uint8_t)) - 1,
    .storage = buffer_data,
    .lock = &app_uart_ringbuffer_lock_dummy,
    .writelock = &buffer_wlock,
    .readlock  = &buffer_rlock
};

#ifndef CEEDLING
static
#endif
void app_uart_init_globs (void)
{
    buffer_wlock = false;
    buffer_rlock = false;
    g_flag_uart_tx_in_progress = false;
    g_resp_type = APP_UART_RESP_TYPE_NONE;
    g_resp_ack_cmd = (re_ca_uart_cmd_t)0;
    g_resp_ack_state = false;
    m_uart_ack = false;
    m_uart_ring_buffer.head = 0;
    m_uart_ring_buffer.tail = 0;
}

/** Dummy function to lock/unlock buffer */
#ifndef CEEDLING
static
#endif
bool app_uart_ringbuffer_lock_dummy (volatile uint32_t * const flag, bool lock)
{
    bool * p_bool = (bool *) flag;

    if (*p_bool == lock) { return false; }

    *p_bool = lock;
    return true;
}

static rd_status_t app_uart_send_msg (ri_comm_message_t * const p_msg)
{
    g_flag_uart_tx_in_progress = true;
    const rd_status_t err_code = m_uart.send (p_msg);

    if (RD_SUCCESS != err_code)
    {
        g_flag_uart_tx_in_progress = false;
    }

    return err_code;
}

static rd_status_t app_uart_send_device_id (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t uart_payload = {0};
    ri_comm_message_t msg = {0};
    msg.data_length = sizeof (msg.data);
    uint64_t mac;
    err_code |= ri_radio_address_get (&mac);
    uint64_t id;
    err_code |= ri_comm_id_get (&id);
    uart_payload.cmd = RE_CA_UART_DEVICE_ID;
    uart_payload.params.device_id.id = id;
    uart_payload.params.device_id.addr = mac;
    err_code |= re_ca_uart_encode (msg.data, &msg.data_length, &uart_payload);
    msg.repeat_count = 1;
    err_code |= app_uart_send_msg (&msg);
    return err_code;
}

static rd_status_t app_uart_send_ack (const re_ca_uart_cmd_t cmd, const bool is_ok)
{
    re_ca_uart_payload_t uart_payload = {0};
    uart_payload.cmd = RE_CA_UART_ACK;
    uart_payload.params.ack.cmd = cmd;

    if (is_ok)
    {
        uart_payload.params.ack.ack_state.state = RE_CA_ACK_OK;
    }
    else
    {
        uart_payload.params.ack.ack_state.state = RE_CA_ACK_ERROR;
    }

    ri_comm_message_t msg = {0};
    msg.data_length = sizeof (msg.data);
    re_status_t err_code = re_ca_uart_encode (msg.data, &msg.data_length, &uart_payload);
    msg.repeat_count = 1;

    if (RE_SUCCESS == err_code)
    {
        err_code |= app_uart_send_msg (&msg);
    }
    else
    {
        err_code |= RD_ERROR_INVALID_DATA;
    }

    return err_code;
}

#ifndef CEEDLING
static
#endif
void app_uart_on_evt_tx_finish (void * p_data, uint16_t data_len)
{
    (void)p_data;
    (void)data_len;
    g_flag_uart_tx_in_progress = false;

    switch (g_resp_type)
    {
        case APP_UART_RESP_TYPE_NONE:
            break;

        case APP_UART_RESP_TYPE_ACK:
            g_resp_type = APP_UART_RESP_TYPE_NONE;
            app_uart_send_ack (g_resp_ack_cmd, g_resp_ack_state);
            return;

        case APP_UART_RESP_TYPE_DEVICE_ID:
            g_resp_type = APP_UART_RESP_TYPE_NONE;
            app_uart_send_device_id();
            return;
    }
}

#ifndef CEEDLING
static
#endif
void app_uart_on_evt_send_device_id (void * p_data, uint16_t data_len)
{
    (void)p_data;
    (void)data_len;
    g_resp_type = APP_UART_RESP_TYPE_DEVICE_ID;

    if (!g_flag_uart_tx_in_progress)
    {
        ri_scheduler_event_put (NULL, (uint16_t)0, app_uart_on_evt_tx_finish);
    }
}

#ifndef CEEDLING
static
#endif
void app_uart_on_evt_send_ack (void * p_data, uint16_t data_len)
{
    (void)p_data;
    (void)data_len;
    g_resp_type = APP_UART_RESP_TYPE_ACK;

    if (!g_flag_uart_tx_in_progress)
    {
        ri_scheduler_event_put (NULL, (uint16_t)0, app_uart_on_evt_tx_finish);
    }
}

#ifndef CEEDLING
static
rd_status_t app_uart_apply_config (re_ca_uart_payload_t * p_uart_payload)
#else
rd_status_t app_uart_apply_config (void * v_uart_payload)
#endif
{
#ifdef CEEDLING
    re_ca_uart_payload_t * p_uart_payload = (re_ca_uart_payload_t *) v_uart_payload;
#endif
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation;
    ri_radio_channels_t channels;

    //    return err_code;

    switch (p_uart_payload->cmd)
    {
        case RE_CA_UART_SET_FLTR_TAGS:
            err_code |= app_ble_manufacturer_filter_set ((bool)
                        p_uart_payload->params.bool_param.state);
            break;

        case RE_CA_UART_SET_FLTR_ID:
            err_code |= app_ble_manufacturer_id_set (p_uart_payload->params.fltr_id_param.id);
            break;

        case RE_CA_UART_SET_CODED_PHY:
            modulation = RI_RADIO_BLE_125KBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.bool_param.state);
            break;

        case RE_CA_UART_SET_SCAN_1MB_PHY:
            modulation = RI_RADIO_BLE_1MBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.bool_param.state);
            break;

        case RE_CA_UART_SET_EXT_PAYLOAD:
            modulation = RI_RADIO_BLE_2MBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.bool_param.state);
            break;

        case RE_CA_UART_SET_CH_37:
            err_code |= app_ble_channels_get (&channels);
            channels.channel_37 = p_uart_payload->params.bool_param.state;
            err_code |= app_ble_channels_set (channels);
            break;

        case RE_CA_UART_SET_CH_38:
            err_code |= app_ble_channels_get (&channels);
            channels.channel_38 = p_uart_payload->params.bool_param.state;
            err_code |= app_ble_channels_set (channels);
            break;

        case RE_CA_UART_SET_CH_39:
            err_code |= app_ble_channels_get (&channels);
            channels.channel_39 = p_uart_payload->params.bool_param.state;
            err_code |= app_ble_channels_set (channels);
            break;

        case RE_CA_UART_SET_ALL:
            err_code |= app_ble_manufacturer_id_set (p_uart_payload->params.all_params.fltr_id.id);
            err_code |= app_ble_manufacturer_filter_set ((bool)
                        p_uart_payload->params.all_params.bools.fltr_tags.state);
            channels.channel_37 = p_uart_payload->params.all_params.bools.ch_37.state;
            channels.channel_38 = p_uart_payload->params.all_params.bools.ch_38.state;
            channels.channel_39 = p_uart_payload->params.all_params.bools.ch_39.state;
            err_code |= app_ble_channels_set (channels);
            modulation = RI_RADIO_BLE_125KBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.all_params.bools.coded_phy.state);
            modulation = RI_RADIO_BLE_1MBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.all_params.bools.scan_phy.state);
            modulation = RI_RADIO_BLE_2MBPS;
            err_code |= app_ble_modulation_enable (modulation,
                                                   (bool) p_uart_payload->params.all_params.bools.ext_payload.state);
            break;

        default:
            err_code |= RE_ERROR_INVALID_PARAM;
            break;
    }

    return err_code;
}
#if 0
#ifndef CEEDLING
static
#endif
void app_uart_repeat_send (void * p_data, uint16_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_comm_message_t msg = {0};
    msg.repeat_count = 1;
    msg.data_length = data_len;
    memcpy (msg.data, p_data, data_len);
    err_code |= app_uart_send_msg (&msg);

    if (RE_SUCCESS != err_code)
    {
        err_code |= ri_scheduler_event_put (msg.data, (uint16_t) msg.data_length,
                                            app_uart_repeat_send);
    }

    if (RD_SUCCESS == err_code)
    {
        ri_watchdog_feed();
    }
}
#endif

#ifndef CEEDLING
static
#endif
void app_uart_parser (void * p_data, uint16_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t uart_payload = {0};
    uint8_t dequeue_data[APP_UART_RING_DEQ_BUFFER_MAX_LEN] = {0};
    rl_status_t status = RL_SUCCESS;
    size_t index = 0;
    err_code = re_ca_uart_decode ((uint8_t *) p_data, &uart_payload);

    if (RD_SUCCESS == err_code)
    {
        do
        {
            uint8_t * p_dequeue_data = NULL;
            status = rl_ringbuffer_dequeue (&m_uart_ring_buffer,
                                            &p_dequeue_data);

            if (NULL != p_dequeue_data)
            {
                dequeue_data[index++] = *p_dequeue_data;
            }
        } while (RL_SUCCESS == status);
    }
    else
    {
        do
        {
            status = rl_ringbuffer_queue (&m_uart_ring_buffer,
                                          (void *) ((uint8_t *) p_data + index),
                                          sizeof (uint8_t));
            index++;
        } while ((RL_SUCCESS == status) && (index < data_len));

        index = 0;

        do
        {
            uint8_t * p_dequeue_data = NULL;
            status = rl_ringbuffer_dequeue (&m_uart_ring_buffer,
                                            &p_dequeue_data);

            if (NULL != p_dequeue_data)
            {
                dequeue_data[index++] = *p_dequeue_data;
            }
        } while (RL_SUCCESS == status);

        err_code = re_ca_uart_decode ((uint8_t *) dequeue_data, &uart_payload);

        if (RD_SUCCESS != err_code)
        {
            size_t len = index;
            index = 0;

            do
            {
                status = rl_ringbuffer_queue (&m_uart_ring_buffer, (void *) (dequeue_data + index),
                                              sizeof (uint8_t));
                index++;
            } while ((RL_SUCCESS == status) && (index < len));
        }
    }

    if (RD_SUCCESS == err_code)
    {
        if (RE_CA_UART_GET_DEVICE_ID == uart_payload.cmd)
        {
            ri_scheduler_event_put (NULL, (uint16_t) 0, app_uart_on_evt_send_device_id);
        }
        else if (RE_CA_UART_LED_CTRL == uart_payload.cmd)
        {
            (void) rt_led_blink_stop (RB_LED_ACTIVITY);

            if (0 != uart_payload.params.led_ctrl_param.time_interval_ms)
            {
                (void) rt_led_blink_once (RB_LED_ACTIVITY,
                                          uart_payload.params.led_ctrl_param.time_interval_ms);
            }

            g_resp_ack_cmd = uart_payload.cmd;
            g_resp_ack_state = true;
            ri_scheduler_event_put (NULL, (uint16_t) 0, app_uart_on_evt_send_ack);
        }
        else
        {
            g_resp_ack_cmd = uart_payload.cmd;

            if (RD_SUCCESS == app_uart_apply_config (&uart_payload))
            {
                g_resp_ack_state = true;
            }
            else
            {
                g_resp_ack_state = false;
            }

            ri_scheduler_event_put (NULL, (uint16_t) 0, app_uart_on_evt_send_ack);

            if (RE_CA_UART_SET_ALL == uart_payload.cmd)
            {
                m_uart_ack = true;
                err_code |= app_ble_scan_start(); // Applies new scanning settings.

                if (RD_SUCCESS == err_code)
                {
                    ri_watchdog_feed();
                }
            }
        }
    }
}

#ifndef CEEDLING
static
#endif
rd_status_t app_uart_isr (ri_comm_evt_t evt,
                          void * p_data, size_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;

    switch (evt)
    {
        case RI_COMM_SENT:
            err_code |= ri_scheduler_event_put (NULL, (uint16_t)0, app_uart_on_evt_tx_finish);
            break;

        case RI_COMM_RECEIVED:
            err_code |= ri_scheduler_event_put (p_data, (uint16_t) data_len, app_uart_parser);
            break;

        default:
            // No action needed on connect/disconnect events.
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
    app_uart_init_globs();
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
    ri_adv_scan_t m_scan = {0};
    ri_comm_message_t msg = {0};
    rd_status_t err_code = RD_SUCCESS;
    re_status_t re_code = RE_SUCCESS;
    uint16_t manuf_id;

    if (NULL == scan)
    {
        err_code |= RD_ERROR_NULL;
    }
    else if (RE_CA_UART_ADV_BYTES >= scan->data_len)
    {
        memcpy (adv.params.adv.mac, scan->addr, sizeof (adv.params.adv.mac));
        memcpy (adv.params.adv.adv, scan->data, scan->data_len);
        memcpy (m_scan.data, scan->data, scan->data_len);
        m_scan.data_len = scan->data_len;
        adv.params.adv.rssi_db = scan->rssi;
        adv.params.adv.adv_len = scan->data_len;
        adv.cmd = RE_CA_UART_ADV_RPRT;
        manuf_id = ri_adv_parse_manuid (m_scan.data, m_scan.data_len);
        uint16_t filter_id = RB_BLE_MANUFACTURER_ID;
        bool flag_discard = false;

        if (app_ble_manufacturer_filter_enabled (&filter_id) &&
                (manuf_id != filter_id))
        {
            flag_discard = true;
        }

        if (flag_discard)
        {
            err_code |= RD_ERROR_INVALID_DATA;
            NRF_LOG_INFO ("app_uart_send_broadcast: discard: manufacturer_id=0x%04x: addr=%s: len=%d",
                          manuf_id,
                          mac_addr_to_str (scan->addr).buf,
                          scan->data_len,
                          scan->primary_phy,
                          scan->secondary_phy,
                          scan->ch_index);
        }
        else
        {
            _Static_assert (sizeof (msg.data) <= UINT8_MAX, "sizeof (msg) <= UINT8_MAX");
            msg.data_length = (uint8_t)sizeof (msg.data);
            re_code = re_ca_uart_encode (msg.data, &msg.data_length, &adv);
            msg.repeat_count = 1;

            if (RE_SUCCESS == re_code)
            {
                NRF_LOG_INFO ("app_uart_send_broadcast: addr=%s: len=%d, primary_phy=%d, secondary_phy=%d, chan=%d, tx_power=%d",
                              mac_addr_to_str (scan->addr).buf,
                              scan->data_len,
                              scan->primary_phy,
                              scan->secondary_phy,
                              scan->ch_index,
                              scan->tx_power);
                NRF_LOG_HEXDUMP_INFO (scan->data, scan->data_len);
                err_code |= app_uart_send_msg (&msg);
            }
            else
            {
                NRF_LOG_ERROR ("%s: re_ca_uart_encode failed", __func__);
                err_code |= RD_ERROR_INVALID_DATA;
            }
        }
    }
    else
    {
        NRF_LOG_ERROR ("%s: addr=%s: data len=%d > RE_CA_UART_ADV_BYTES (%d)",
                       __func__,
                       mac_addr_to_str (scan->addr).buf,
                       scan->data_len, RE_CA_UART_ADV_BYTES);
        err_code |= RD_ERROR_DATA_SIZE;
    }

    return err_code;
}

rd_status_t app_uart_poll_configuration (void)
{
    re_ca_uart_payload_t cfg = {0};
    ri_comm_message_t msg = {0};
    rd_status_t err_code = RD_SUCCESS;
    re_status_t re_code = RE_SUCCESS;
    msg.data_length = sizeof (msg.data);
    cfg.cmd = RE_CA_UART_GET_ALL;
    re_code = re_ca_uart_encode (msg.data, &msg.data_length, &cfg);
    msg.repeat_count = RI_COMM_MSG_REPEAT_FOREVER;

    if (RE_SUCCESS == re_code)
    {
        err_code |= app_uart_send_msg (&msg);

        do
        {
            ri_scheduler_execute();
            ri_yield();
        } while (!m_uart_ack);
    }
    else
    {
        err_code |= RD_ERROR_INVALID_DATA;
    }

    return err_code;
}

/** @} */
