/**
 *  @file app_ble.c
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-12
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application BLE control, selecting PHYs and channels to scan on.
 */

#include "app_ble.h"
#include "app_uart.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_boards.h"
#include "ruuvi_interface_log.h"
#include "ruuvi_interface_communication.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_gpio.h"
#include "ruuvi_interface_scheduler.h"
#include "ruuvi_interface_watchdog.h"
#include "ruuvi_task_advertisement.h"

#include <string.h>

static inline void LOG (const char * const msg)
{
    ri_log (RI_LOG_LEVEL_INFO, msg);
}

static inline void LOGD (const char * const msg)
{
    ri_log (RI_LOG_LEVEL_DEBUG, msg);
}


static app_ble_scan_t m_scan_params;  //!< Configured scan

#ifndef CEEDLING
static
#endif
void repeat_adv (void * p_data, uint16_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;

    if (sizeof (ri_adv_scan_t) == data_len)
    {
        err_code |= app_uart_send_broadcast ( (ri_adv_scan_t *) p_data);

        if (RD_SUCCESS == err_code)
        {
            ri_watchdog_feed();
        }
    }
}

/**
 * @brief Handle Scan events.
 *
 * Received data is put to scheduler queue, new scan with new PHY is started on timeout.
 *
 * @param[in] evt Type of event, either RI_COMM_RECEIVED on data or
 *                RI_COMM_TIMEOUT on scan timeout.
 * @param[in] p_data NULL on timeout, ri_adv_scan_t* on received.
 * @param[in] data_len 0 on timeout, size of ri_adv_scan_t on received.
 * @retval RD_SUCCESS on successful handling on event.
 * @retval RD_ERR_NO_MEM if received event could not be put to scheduler queue.
 * @return Error code from scanning if scan cannot be started.
 *
 * @note parameters are not const to maintain compatibility with the event handler
 *       signature.
 **/
#ifndef CEEDLING
static
#endif
rd_status_t on_scan_isr (const ri_comm_evt_t evt, void * p_data, // -V2009
                         size_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;

    switch (evt)
    {
        case RI_COMM_RECEIVED:
            LOGD ("DATA\r\n");
            err_code |= ri_scheduler_event_put (p_data, (uint16_t) data_len, repeat_adv);
            break;

        case RI_COMM_TIMEOUT:
            LOGD ("Timeout\r\n");
            err_code |= app_ble_scan_start();
            break;

        default:
            LOG ("Unknown event\r\n");
            break;
    }

    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);
    return err_code;
}

rd_status_t app_ble_manufacturer_filter_set (const bool state)
{
    rd_status_t  err_code = RD_SUCCESS;
    m_scan_params.manufacturer_filter_enabled = state;
    return err_code;
}

rd_status_t app_ble_manufacturer_id_set (const uint16_t id)
{
    rd_status_t  err_code = RD_SUCCESS;
    m_scan_params.manufacturer_id = id;
    return err_code;
}

ri_radio_channels_t app_ble_channels_get (void)
{
    return m_scan_params.scan_channels;
}

rd_status_t app_ble_channels_set (const ri_radio_channels_t channels)
{
    rd_status_t err_code = RD_SUCCESS;

    if ( (0 == channels.channel_37)
            && (0 == channels.channel_38)
            && (0 == channels.channel_39))
    {
        err_code |= RD_ERROR_INVALID_PARAM;
    }
    else
    {
        m_scan_params.scan_channels = channels;
    }

    return err_code;
}

rd_status_t app_ble_modulation_enable (const ri_radio_modulation_t modulation,
                                       const bool enable)
{
    rd_status_t err_code = RD_SUCCESS;

    switch (modulation)
    {
        case RI_RADIO_BLE_125KBPS:
            if (RB_BLE_CODED_SUPPORTED)
            {
                m_scan_params.modulation_125kbps_enabled = enable;
            }
            else
            {
                err_code |= RD_ERROR_NOT_SUPPORTED;
            }

            break;

        case RI_RADIO_BLE_1MBPS:
            m_scan_params.modulation_1mbit_enabled = enable;
            break;

        case RI_RADIO_BLE_2MBPS:
            m_scan_params.modulation_2mbit_enabled = enable;
            break;

        default:
            err_code |= RD_ERROR_INVALID_PARAM;
            break;
    }

    return err_code;
}

static inline void next_modulation_select (void)
{
    switch (m_scan_params.current_modulation)
    {
        case RI_RADIO_BLE_125KBPS:
            if (m_scan_params.modulation_1mbit_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_1MBPS;
            }
            else if (m_scan_params.modulation_2mbit_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_2MBPS;
            }
            else
            {
                // No action needed.
            }

            break;

        case RI_RADIO_BLE_1MBPS:
            if (m_scan_params.modulation_2mbit_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_2MBPS;
            }
            else if (m_scan_params.modulation_125kbps_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_125KBPS;
            }
            else
            {
                // No action needed.
            }

            break;

        case RI_RADIO_BLE_2MBPS:
            if (m_scan_params.modulation_125kbps_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_125KBPS;
            }
            else if (m_scan_params.modulation_1mbit_enabled)
            {
                m_scan_params.current_modulation = RI_RADIO_BLE_1MBPS;
            }
            else
            {
                // No action needed.
            }

            break;

        default:
            // No action needed.
            break;
    }
}

static rd_status_t pa_lna_ctrl (void)
{
    rd_status_t err_code = RD_SUCCESS;
#if RB_PA_ENABLED

    if (!ri_gpio_is_init())
    {
        err_code |= ri_gpio_init();
    }

    // Allow ESP32 to force LNA off for WiFi TX bursts
    err_code |= ri_gpio_configure (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP);
    err_code |= ri_gpio_configure (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD);
    err_code |= ri_gpio_write (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE);
#endif
    return err_code;
}

rd_status_t app_ble_scan_start (void)
{
    rd_status_t err_code = RD_SUCCESS;
    err_code |= rt_adv_uninit();
    err_code |= ri_radio_uninit();
    rt_adv_init_t adv_params =
    {
        .channels = m_scan_params.scan_channels,
        .adv_interval_ms = (1000U), //!< Unused
        .adv_pwr_dbm     = (0),     //!< Unused
        .manufacturer_id = RB_BLE_MANUFACTURER_ID //!< default
    };

    if (RD_SUCCESS == err_code)
    {
        next_modulation_select();
        err_code |= pa_lna_ctrl();
        err_code |= ri_radio_init (m_scan_params.current_modulation);

        if (RD_SUCCESS == err_code)
        {
            err_code |= rt_adv_init (&adv_params);
            err_code |= rt_adv_scan_start (&on_scan_isr);
            err_code |= ri_watchdog_feed ();
        }
    }

    return err_code;
}
