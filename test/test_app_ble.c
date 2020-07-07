#include "unity.h"

#include "app_ble.h"
#include "ruuvi_boards.h"
#include "mock_app_uart.h"
#include "mock_ruuvi_driver_error.h"
#include "mock_ruuvi_interface_communication_radio.h"
#include "mock_ruuvi_interface_gpio.h"
#include "mock_ruuvi_interface_log.h"
#include "mock_ruuvi_interface_scheduler.h"
#include "mock_ruuvi_interface_watchdog.h"
#include "mock_ruuvi_task_advertisement.h"

void setUp (void)
{
    ri_log_Ignore();
    const ri_radio_channels_t channels =
    {
        .channel_37 = 1,
        .channel_38 = 1,
        .channel_39 = 1
    };
    app_ble_channels_select (channels);
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, false);
    app_ble_modulation_enable (RI_RADIO_BLE_1MBPS, false);
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, false);
}

void tearDown (void)
{
}

ri_adv_scan_t mock_scan =
{
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    .rssi = -55,
    {'A', 'v', 'e', ' ', 'M', 'u', 'n', 'd', 'i', 0},
    .data_len = 10
};

size_t mock_scan_len = sizeof (mock_scan);

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

void test_app_ble_channels_one_ch (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels =
    {
        .channel_39 = 1
    };
    err_code |= app_ble_channels_select (channels);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_channels_two_ch (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels =
    {
        .channel_38 = 1,
        .channel_39 = 1
    };
    err_code |= app_ble_channels_select (channels);
    TEST_ASSERT (RD_SUCCESS == err_code);
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
void test_app_ble_modulation_1mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation = RI_RADIO_BLE_1MBPS;
    err_code |= app_ble_modulation_enable (modulation, true);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_modulation_2mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation = RI_RADIO_BLE_2MBPS;
    err_code |= app_ble_modulation_enable (modulation, true);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

// TODO: This should return RD_ERROR_NOT_SUPPORTED on boards with 52832
void test_app_ble_modulation_125kbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation = RI_RADIO_BLE_125KBPS;
    err_code |= app_ble_modulation_enable (modulation, true);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_modulation_invalid (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_radio_modulation_t modulation = (ri_radio_modulation_t) 53;
    err_code |= app_ble_modulation_enable (modulation, true);
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
void test_app_ble_scan_start_all_modulations_channels (void)
{
    rd_status_t err_code = RD_SUCCESS;
    static rt_adv_init_t adv_params =
    {
        .channels =
        {
            .channel_37 = 1,
            .channel_38 = 1,
            .channel_39 = 1
        },
        .adv_interval_ms = (1000U),
        .adv_pwr_dbm     = (0),                         //!< Unused
        .manufacturer_id = RB_BLE_MANUFACTURER_ID       //!< Default
    };
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_1MBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_1MBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&adv_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (true);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_2MBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&adv_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (true);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_125KBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&adv_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
}
