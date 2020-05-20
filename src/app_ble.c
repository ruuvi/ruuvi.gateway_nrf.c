/** 
 *  @file app_ble.c
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-12
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application BLE control, selecting PHYs and channels to scan on.
 */
 
#include "app_ble.h"
#include "uart.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_boards.h"
#include "ruuvi_interface_log.h"
#include "ruuvi_interface_communication.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_interface_communication_ble_advertising.h"
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


app_ble_scan_t m_scan_params;  //!< Configured scan
app_ble_on_scan_fp_t m_on_ble; //!< Callback for scans

/** @brief Pass scan data on application via this FP */
void app_ble_set_on_scan(const app_ble_on_scan_fp_t callback)
{
    m_on_ble = callback;
}

/** @brief Handle driver events */
static rd_status_t on_scan_isr(const ri_comm_evt_t evt, void * p_data, size_t data_len)
{
    rd_status_t err_code = RD_SUCCESS;
    switch(evt)
    {
        case RI_COMM_RECEIVED:
          LOGD("DATA\r\n");
          break;

        case RI_COMM_TIMEOUT:
          LOG("Timeout\r\n");
          err_code |= app_ble_scan_start();
          break;
        
        default:
          LOG("Unknown event\r\n");
          break;
    }
    return err_code;
}

/**
 * @brief Enable or disable given channels.
 *
 * On 2 MBit / s modulation, the channels only enable / disable
 * scanning for primary advertisement at 1 MBit / s rate.
 * The extended payload at 2 MBit / s.
 *
 * @param[in] enabled_channels Channels to enable.
 * @retval RD_SUCCESS on success.
 * @retval RD_ERROR_INVALID_PARAM If no channels are enabled.
 */
rd_status_t app_ble_channels_select(const ri_radio_channels_t channels)
{
    rd_status_t err_code = RD_SUCCESS;
    if(memcmp(&channels, 0, sizeof(channels)))
    {
        err_code |= RD_ERROR_INVALID_PARAM;
    }
    else
    {
        m_scan_params.scan_channels = channels;
    }
    return err_code;
}

/**
 * @brief Enable or disable given modulation.
 *
 * @param[in] modulation Modulation to enable / disable.
 * @param[in] enable True to enable, false to disable.
 * @retval RD_SUCCESS on success.
 * @retval RD_ERROR_INVALID_PARAM If given invalid modulation. 
 * @retval RD_ERROR_NOT_SUPPORTED If given modulation not supported by board.
 */
rd_status_t app_ble_modulation_enable(const ri_radio_modulation_t modulation, const bool enable)
{
    rd_status_t err_code = RD_SUCCESS;
    switch(modulation)
    {
        case RI_RADIO_BLE_125KBPS:
            if(RB_BLE_CODED_SUPPORTED)
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

static inline void next_modulation_select(void)
{
    switch(m_scan_params.current_modulation)
    {
        case RI_RADIO_BLE_125KBPS:
            if(m_scan_params.modulation_1mbit_enabled)
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
            if(m_scan_params.modulation_2mbit_enabled)
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
            if(m_scan_params.modulation_125kbps_enabled)
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

/**
 * @brief Start a scan sequence.
 *
 * Runs through enabled modulations and channels, automatically starting
 * on next after one has finished. Calls given scan callback on data.
 *
 * @retval RD_SUCCESS on success.
 *
 */
rd_status_t app_ble_scan_start(void)
{
    rd_status_t err_code = RD_SUCCESS;
    err_code |= ri_radio_uninit();
    rt_adv_init_t adv_params = 
    {
        .channels = m_scan_params.scan_channels,
        .adv_interval_ms = (1000U), //!< Unused
        .adv_pwr_dbm     = (0),     //!< Unused
        .manufacturer_id = RB_BLE_MANUFACTURER_ID //!< default
    };
    if(RD_SUCCESS == err_code)
    {
        next_modulation_select();
        err_code |= ri_radio_init(m_scan_params.current_modulation);
        if(RD_SUCCESS == err_code)
        {
            err_code |= rt_adv_init(&adv_params);
            err_code |= rt_adv_scan_start(&on_scan_isr);
            err_code |= ri_watchdog_feed();
        }
    }
    return err_code;
}

#ifdef CEEDLING
rd_status_t on_scan(const ri_comm_evt_t evt, void * p_data, size_t data_len);
#endif
