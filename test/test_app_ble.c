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
#include <string.h>

void setUp (void)
{
    ri_log_Ignore();
    rd_error_check_Ignore();
    const ri_radio_channels_t channels =
    {
        .channel_37 = 1,
        .channel_38 = 1,
        .channel_39 = 1
    };
    app_ble_manufacturer_filter_set (true);
    app_ble_manufacturer_id_set (RB_BLE_MANUFACTURER_ID);
    app_ble_channels_set (channels);
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
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

static rt_adv_init_t scan_params =
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
    err_code |= app_ble_channels_set (channels);
    TEST_ASSERT (RD_ERROR_INVALID_PARAM == err_code);
}

void test_app_ble_channels_one_ch (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels =
    {
        .channel_39 = 1
    };
    ri_radio_channels_t get_channels;
    err_code |= app_ble_channels_set (channels);
    err_code |= app_ble_channels_get (&get_channels);
    TEST_ASSERT (RD_SUCCESS == err_code);
    TEST_ASSERT (0 == get_channels.channel_37 &&
                 0 == get_channels.channel_38 &&
                 1 == get_channels.channel_39);
}

void test_app_ble_channels_two_ch (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_radio_channels_t channels =
    {
        .channel_38 = 1,
        .channel_39 = 1
    };
    ri_radio_channels_t get_channels;
    err_code |= app_ble_channels_set (channels);
    err_code |= app_ble_channels_get (&get_channels);
    TEST_ASSERT (RD_SUCCESS == err_code);
    TEST_ASSERT (0 == get_channels.channel_37 &&
                 1 == get_channels.channel_38 &&
                 1 == get_channels.channel_39);
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
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_1MBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (true);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_125KBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_1MBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
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
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_scan_start_all_channels_lr (void)
{
    rd_status_t err_code = RD_SUCCESS;
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_125KBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_scan_start_all_channels_1mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    app_ble_modulation_enable (RI_RADIO_BLE_1MBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_1MBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_scan_start_all_channels_2mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_2MBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_scan_start_all_channels_lr_2mbps (void)
{
    rd_status_t err_code = RD_SUCCESS;
    app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, true);
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAndReturn (RI_RADIO_BLE_125KBPS, RD_SUCCESS);
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
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
    rt_adv_init_ExpectWithArrayAndReturn (&scan_params, 1, RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_ble_scan_start();
    TEST_ASSERT (RD_SUCCESS == err_code);
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

void test_app_ble_on_scan_isr_received (void)
{
    rd_status_t err_code = RD_SUCCESS;
    char received[] = "Ave Mundi!";
    ri_scheduler_event_put_ExpectAndReturn (received, strlen (received), &repeat_adv,
                                            RD_SUCCESS);
    err_code |= on_scan_isr (RI_COMM_RECEIVED, received, strlen (received));
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_on_scan_isr_timeout (void)
{
    rd_status_t err_code = RD_SUCCESS;
    rt_adv_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_radio_uninit_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_is_init_ExpectAndReturn (false);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CRX_PIN, RI_GPIO_MODE_INPUT_PULLUP, RD_SUCCESS);
    ri_gpio_configure_ExpectAndReturn (RB_PA_CSD_PIN, RI_GPIO_MODE_OUTPUT_STANDARD,
                                       RD_SUCCESS);
    ri_gpio_write_ExpectAndReturn (RB_PA_CSD_PIN, RB_PA_CSD_ACTIVE, RD_SUCCESS);
    ri_radio_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    rt_adv_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    rt_adv_scan_start_ExpectAndReturn (&on_scan_isr, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    err_code |= on_scan_isr (RI_COMM_TIMEOUT, NULL, 0);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_app_ble_on_scan_isr_unknown (void)
{
    rd_status_t err_code = RD_SUCCESS;
    err_code |= on_scan_isr (RI_COMM_SENT, NULL, 0);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_repeat_adv_ok (void)
{
    rd_status_t err_code = RD_SUCCESS;
    app_uart_send_broadcast_ExpectAndReturn (&mock_scan, RD_SUCCESS);
    ri_watchdog_feed_ExpectAndReturn (RD_SUCCESS);
    repeat_adv (&mock_scan, mock_scan_len);
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_repeat_adv_incorrect_len (void)
{
    rd_status_t err_code = RD_SUCCESS;
    repeat_adv (&mock_scan, (mock_scan_len - 1));
    TEST_ASSERT (RD_SUCCESS == err_code);
}

void test_repeat_adv_send_error (void)
{
    rd_status_t err_code = RD_ERROR_DATA_SIZE;
    app_uart_send_broadcast_ExpectAndReturn (NULL, RD_ERROR_DATA_SIZE);
    repeat_adv (NULL, mock_scan_len);
    TEST_ASSERT (RD_ERROR_DATA_SIZE == err_code);
}
