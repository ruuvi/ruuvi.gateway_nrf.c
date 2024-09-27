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
    uint16_t manufacturer_id;          //!< Set to manufacturer id to scan, in MSB format. e.g. 0x0499 for Ruuvi.
    ri_radio_channels_t scan_channels; //!< Channels to scan, not applicable on 2 MBit / s
    bool modulation_125kbps_enabled;   //!< True to enable scanning BLE Long Range
    bool modulation_1mbit_enabled;     //!< True to enable "classic" scanning
    bool modulation_2mbit_enabled;     //!< True to enable scanning for extended advs at 2 MBit/s.
    bool manufacturer_filter_enabled;  //!< True to scan only data of one manufacturer.
    bool is_current_modulation_125kbps; //!< Modulation used currently.
} app_ble_scan_t;

/**
 * @brief Enable or disable id filter.
 *
 * @param[in] state Filter enable / disable.
 * @retval RD_SUCCESS on success.
 */
rd_status_t app_ble_manufacturer_filter_set (const bool state);

/**
 * @brief Set id filter.
 *
 * @param[in] id New id to set.
 * @retval RD_SUCCESS on success.
 */
rd_status_t app_ble_manufacturer_id_set (const uint16_t id);

/**
 * @brief Get current state of chznnels.
 *
 * @param[in] p_channels Pointer to channels to enable.
 * @retval RD_SUCCESS on success.
 */
rd_status_t app_ble_channels_get (ri_radio_channels_t * p_channels);

/**
 * @brief Enable or disable given channels.
 *
 * On 2 MBit / s modulation, the channels only enable / disable
 * scanning for primary advertisement at 1 MBit / s rate.
 * The extended payload at 2 MBit / s.
 *
 * @param[in] channels Channels to enable.
 * @retval RD_SUCCESS on success.
 * @retval RD_ERROR_INVALID_PARAM If no channels are enabled.
 */
rd_status_t app_ble_channels_set (const ri_radio_channels_t channels);

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
 * Checks if any PHY is enabled. Runs through enabled modulations and channels,
 * automatically starting on next after one has finished. Calls given scan
 * callback on data. If all PHYs are disabled, calls app_ble_scan_stop() to
 * stop scanning process until any PHY is reactivated.
 *
 * @retval RD_SUCCESS on success.
 *
 */
rd_status_t app_ble_scan_start (void);


/**
 * @brief Check enabled Manufacturer ID filter.
 *
 * Function returns the status of enabled manufacturer ID scan filter.
 *
 * @param[out] p_manufacturer_id - ptr to the variable where
 *             manufactured_id will be stored
 *
 * @retval TRUE If filter is enabled.
 */
bool app_ble_manufacturer_filter_enabled (uint16_t * const p_manufacturer_id);

/**
 * @brief Stop a scan sequence.
 *
 * Aborts the scanning process.
 *
 * @retval RD_SUCCESS on success.
 * @retval RD_ERROR_INVALID_STATE if scan is not initialized.
 */
rd_status_t app_ble_scan_stop (void);

#ifdef CEEDLING
rd_status_t on_scan_isr (const ri_comm_evt_t evt, void * p_data, // -V2009
                         size_t data_len);
void repeat_adv (void * p_data, uint16_t data_len);
#endif

#endif
