#include "unity.h"

#include "app_ble.h"
#include "mock_ruuvi_driver_error.h"
#include "mock_ruuvi_interface_communication_radio.h"
#include "mock_ruuvi_task_advertisement.h"

size_t num_mock_scans;

void setUp (void)
{
    num_mock_scans = 0;
}

void tearDown (void)
{
    app_ble_set_on_scan (NULL);
}

ri_adv_scan_t mock_scan =
{
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    .rssi = -55,
    {'A', 'v', 'e', ' ', 'M', 'u', 'n', 'd', 'i', 0},
    .data_len = 10
};

size_t mock_scan_len = sizeof (mock_scan);

void mock_on_scan (ri_adv_scan_t * const scan)
{
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

void test_app_ble_channels_invalid (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels = { 0 };
    err_code |= app_ble_channels_select (channels);
    TEST_ASSERT (RD_ERROR_INVALID_PARAM == err_code);
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
void test_app_ble_modulation_invalid (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation = (ri_radio_modulation_t) 53;
    app_ble_modulation_enable (modulation, true);
    TEST_ASSERT (RD_ERROR_INVALID_PARAM == err_code);
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
void test_app_ble_scan_1mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels =
    {
        1,
        1,
        1
    };
    err_code |= app_ble_modulation_enable (RI_RADIO_BLE_1MBPS, true);
    err_code |= app_ble_channels_select (channels);
    on_scan (RI_COMM_RECEIVED, mock_scan, mock_scan_len);
}