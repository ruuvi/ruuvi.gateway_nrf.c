#ifndef BLE_H_
#define BLE_H_

/**
 *  @file app_ble.h
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-11
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application BLE control, selecting PHYs and channels to scan on.
 */

#include <stdint.h>
#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_interface_communication_ble_advertising.h"

/** @brief definition of application scan parameters */
typedef struct
{
    ri_radio_channels_t scan_channels; //!< Channels to scan, not applicable on 2 MBit / s
    bool modulation_125kbps_enabled;   //!< True to enable scanning BLE Long Range
    bool modulation_1mbit_enabled;     //!< True to enable "classic" scanning
    bool modulation_2mbit_enabled;     //!< True to enable scanning for extended advs at 2 MBit/s.
    bool manufacturer_filter_enabled;  //!< True to scan only data of one manufacturer.
    uint16_t manufacturer_id;          //!< Set to manufacturer id to scan, in MSB format. e.g. 0x0499 for Ruuvi.
    ri_radio_modulation_t current_modulation; //!< Modulation used currently.
} app_ble_scan_t;

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
rd_status_t app_ble_channels_select (const ri_radio_channels_t channels);

/**
 * @brief Enable or disable given modulation.
 *
 * @param[in] modulation Modulation to enable / disable.
 * @param[in] enable True to enable, false to disable.
 * @retval RD_SUCCESS on success.
 * @retval RD_ERROR_INVALID_PARAM If given invalid modulation.
 * @retval RD_ERROR_NOT_SUPPORTED If given modulation not supported by board.
 */
rd_status_t app_ble_modulation_enable (const ri_radio_modulation_t modulation,
                                       const bool enable);

/**
 * @brief Start a scan sequence.
 *
 * Runs through enabled modulations and channels, automatically starting
 * on next after one has finished. Calls given scan callback on data.
 *
 * @retval RD_SUCCESS on success.
 *
 */
rd_status_t app_ble_scan_start (void);

#ifdef CEEDLING
rd_status_t on_scan (const ri_comm_evt_t evt, void * p_data, size_t data_len);
#endif

#endif
