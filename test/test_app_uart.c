#include "unity.h"

#include "ble_gap.h"
#include "app_uart.h"
#include "mock_app_ble.h"
#include "ruuvi_boards.h"
#include "mock_ruuvi_interface_communication_ble_advertising.h"
#include "mock_ruuvi_interface_communication.h"
#include "mock_ruuvi_interface_communication_radio.h"
#include "mock_ruuvi_driver_error.h"
#include "mock_ruuvi_endpoint_ca_uart.h"
#include "mock_ruuvi_interface_communication_uart.h"
#include "mock_ruuvi_interface_scheduler.h"
#include "mock_ruuvi_interface_yield.h"
#include "mock_ruuvi_interface_watchdog.h"
#include "mock_ruuvi_library_ringbuffer.h"
#include "mock_ruuvi_task_led.h"

#include <string.h>

/** https://stackoverflow.com/questions/3385515/static-assert-in-c, Nordic Mainframe */
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]

/** https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c Joey Adams*/
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

STATIC_ASSERT (MEMBER_SIZE (ri_adv_scan_t, addr) == MEMBER_SIZE (re_ca_uart_ble_adv_t,
               mac), \
               Size_mismatch);

#define MOCK_MAC_ADDR_INIT() {0xFA, 0xEB, 0xDC, 0xCD, 0xBE, 0xAF}
const uint8_t mock_mac[BLE_MAC_ADDRESS_LENGTH] = MOCK_MAC_ADDR_INIT();
#define MOCK_DATA_INIT() \
    { \
        0x05U, 0x0FU, 0x27U, 0x40U, \
        0x35U, 0xC4U, 0x54U, 0x54U, 0x50U, 0x00U, 0xC8U, 0xFCU, \
        0x20U, 0xA4U, 0x56U, 0xF0U, 0x30U, 0xE5U, 0xC9U, 0x44U, 0x54U, 0x29U, 0xE3U, \
        0x8DU \
    }
const uint8_t mock_data[] = MOCK_DATA_INIT();
const uint16_t mock_manuf_id = 0x0499;

static uint8_t t_ring_buffer[128] = {0};
static bool t_buffer_wlock = false;
static bool t_buffer_rlock = false;

extern volatile bool m_uart_ack;

static rl_ringbuffer_t t_uart_ring_buffer =
{
    .head = 0,
    .tail = 0,
    .block_size = sizeof (uint8_t),
    .storage_size = sizeof (t_ring_buffer),
    .index_mask = (sizeof (t_ring_buffer) / sizeof (uint8_t)) - 1,
    .storage = t_ring_buffer,
    .lock = app_uart_ringbuffer_lock_dummy,
    .writelock = &t_buffer_wlock,
    .readlock  = &t_buffer_rlock
};

static size_t mock_sends = 0;
// Mock sending fp for data through uart.
static rd_status_t mock_send (ri_comm_message_t * const msg)
{
    mock_sends++;
    return RD_SUCCESS;
}

static rd_status_t dummy_send_success (ri_comm_message_t * const msg)
{
    m_uart_ack = true;
    return RD_SUCCESS;
}

static rd_status_t dummy_send_fail (ri_comm_message_t * const msg)
{
    m_uart_ack = false;
    return RD_SUCCESS;
}


// Send function that returns an error to trigger error path in app_uart_send_msg
static rd_status_t dummy_send_error (ri_comm_message_t * const msg)
{
    (void)msg;
    return RD_ERROR_INTERNAL;
}


static ri_comm_channel_t mock_uart =
{
    .send = &mock_send,
    .on_evt = app_uart_isr
};

static ri_comm_channel_t dummy_uart_success =
{
    .send = &dummy_send_success,
    .on_evt = app_uart_isr
};

static ri_comm_channel_t dummy_uart_fail =
{
    .send = &dummy_send_fail,
    .on_evt = app_uart_isr
};

static ri_comm_channel_t dummy_uart_error =
{
    .send = &dummy_send_error,
    .on_evt = app_uart_isr
};

// --- Unit tests for app_uart_ringbuffer_lock_dummy ---
void test_app_uart_ringbuffer_lock_sets_true_from_false (void)
{
    volatile uint32_t flag = 0U;
    bool changed = app_uart_ringbuffer_lock_dummy (&flag, true);
    TEST_ASSERT_TRUE (changed);
    TEST_ASSERT_EQUAL_UINT32 (1U, flag);
}

void test_app_uart_ringbuffer_lock_no_change_when_already_true (void)
{
    volatile uint32_t flag = 1U;
    bool changed = app_uart_ringbuffer_lock_dummy (&flag, true);
    TEST_ASSERT_FALSE (changed);
    TEST_ASSERT_EQUAL_UINT32 (1U, flag);
}

void test_app_uart_ringbuffer_lock_sets_false_from_true (void)
{
    volatile uint32_t flag = 1U;
    bool changed = app_uart_ringbuffer_lock_dummy (&flag, false);
    TEST_ASSERT_TRUE (changed);
    TEST_ASSERT_EQUAL_UINT32 (0U, flag);
}

void test_app_uart_ringbuffer_lock_no_change_when_already_false (void)
{
    volatile uint32_t flag = 0U;
    bool changed = app_uart_ringbuffer_lock_dummy (&flag, false);
    TEST_ASSERT_FALSE (changed);
    TEST_ASSERT_EQUAL_UINT32 (0U, flag);
}



void setUp (void)
{
    mock_sends = 0;
    app_uart_init_globs();
}

void tearDown (void)
{
}

/**
 * @brief Initialize UART peripheral with values read from ruuvi_boards.h.
 *
 * After initialization UART peripheral is active and ready to handle incoming data
 * and send data out.
 *
 * @retval RD_SUCCESS On successful init.
 * @retval RD_ERROR_INVALID_STATE If UART is already initialized.
 */
void test_app_uart_init_ok (void)
{
    static ri_uart_init_t config =
    {
        .hwfc_enabled = RB_HWFC_ENABLED,
        .parity_enabled = RB_PARITY_ENABLED,
        .cts  = RB_UART_CTS_PIN,
        .rts  = RB_UART_RTS_PIN,
        .tx   = RB_UART_TX_PIN,
        .rx   = RB_UART_RX_PIN,
        .baud = RI_UART_BAUD_115200 //!< XXX hardcoded, should come from board.
    };
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_uart_init_ReturnThruPtr_channel (&mock_uart);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    rd_status_t err_code = app_uart_init();
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}
void test_app_uart_init_twice (void)
{
    ri_uart_init_t config = {0};
    memset (&config, 0, sizeof (ri_uart_init_t));
    config.hwfc_enabled = RB_HWFC_ENABLED;
    config.parity_enabled = RB_PARITY_ENABLED;
    config.cts  = RB_UART_CTS_PIN;
    config.rts  = RB_UART_RTS_PIN;
    config.tx   = RB_UART_TX_PIN;
    config.rx   = RB_UART_RX_PIN;
    config.baud = RI_UART_BAUD_115200; //!< XXX hardcoded, should come from board.
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    rd_status_t err_code = app_uart_init();
    ri_uart_init_ExpectAnyArgsAndReturn (RD_ERROR_INVALID_STATE);
    err_code |= app_uart_init();
    TEST_ASSERT (RD_ERROR_INVALID_STATE == err_code);
}

/**
 * @brief app_uart_on_evt_tx_finish with no response pending should do nothing.
 *
 * Verifies the NONE branch: no scheduler calls and no UART sends are triggered.
 */
void test_app_uart_on_evt_tx_finish_none_does_nothing (void)
{
    // Ensure UART is initialized so that any unintended send would be captured by mock_sends.
    test_app_uart_init_ok();
    size_t prev_sends = mock_sends;
    // No expectations for scheduler event put here: if it gets called, test will fail.
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (prev_sends, mock_sends);
}

/**
 * @brief Send a scanned BLE broadcast through UART.
 *
 * The format is defined by ruuvi.endpoints.c/
 *
 * @param[in] scan Scan result from BLE advertisement module.
 * @retval RD_SUCCESS If encoding and queuing data to UART was successful.
 * @retval RD_ERROR_NULL If scan was NULL.
 * @retval RD_ERROR_INVALID_DATA If scan cannot be encoded for any reason.
 * @retval RD_ERROR_DATA_SIZE If scan had larger advertisement size than allowed by
 *                            encoding module.
 */
void test_app_uart_send_broadcast_ok_regular (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_adv_scan_t scan =
    {
        .addr = MOCK_MAC_ADDR_INIT(),
        .rssi = -50,
        .data = MOCK_DATA_INIT(),
        .data_len = sizeof (mock_data),
        .is_coded_phy = false,
        .primary_phy = RE_CA_UART_BLE_PHY_1MBPS,
        .secondary_phy = RE_CA_UART_BLE_PHY_NOT_SET,
        .ch_index = 37,
        .tx_power = BLE_GAP_POWER_LEVEL_INVALID,
    };
    test_app_uart_init_ok();
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_send_broadcast (&scan); // Call the function under test
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// In app_uart_on_evt_send_device_id cover the false case of
// if (!g_flag_uart_tx_in_progress): when TX is in progress, it must NOT schedule tx finish
void test_app_uart_on_evt_send_device_id_when_tx_in_progress_does_not_schedule (void)
{
    // Initialize UART
    test_app_uart_init_ok();
    // Trigger a successful send to set g_flag_uart_tx_in_progress = true
    const ri_adv_scan_t scan =
    {
        .addr = MOCK_MAC_ADDR_INIT(),
        .rssi = -50,
        .data = MOCK_DATA_INIT(),
        .data_len = sizeof (mock_data),
        .is_coded_phy = false,
        .primary_phy = RE_CA_UART_BLE_PHY_1MBPS,
        .secondary_phy = RE_CA_UART_BLE_PHY_NOT_SET,
        .ch_index = 37,
        .tx_power = BLE_GAP_POWER_LEVEL_INVALID,
    };
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    TEST_ASSERT_EQUAL (RD_SUCCESS, app_uart_send_broadcast (&scan));
    TEST_ASSERT_EQUAL (1, mock_sends);
    // Now TX is in progress. Calling app_uart_on_evt_send_device_id should NOT
    // schedule app_uart_on_evt_tx_finish. We deliberately set no expectation for
    // ri_scheduler_event_put so any call would fail the test.
    app_uart_on_evt_send_device_id (NULL, 0);
}

// Cover branch in app_uart_send_msg: if (RD_SUCCESS != err_code)
void test_app_uart_send_msg_error_path (void)
{
    // Initialize app_uart with a UART channel whose send returns an error
    static ri_uart_init_t config =
    {
        .hwfc_enabled = RB_HWFC_ENABLED,
        .parity_enabled = RB_PARITY_ENABLED,
        .cts  = RB_UART_CTS_PIN,
        .rts  = RB_UART_RTS_PIN,
        .tx   = RB_UART_TX_PIN,
        .rx   = RB_UART_RX_PIN,
        .baud = RI_UART_BAUD_115200
    };
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    // Inject our erroring UART channel
    ri_uart_init_ReturnThruPtr_channel (&dummy_uart_error);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    TEST_ASSERT_EQUAL (RD_SUCCESS, app_uart_init());
    // Encode succeeds so that app_uart_poll_configuration will attempt to send
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    // Call function that internally calls app_uart_send_msg and should return error
    rd_status_t err_code = app_uart_poll_configuration();
    TEST_ASSERT_EQUAL (RD_ERROR_INTERNAL, err_code);
    // Verify that TX in-progress flag was cleared by checking that
    // app_uart_on_evt_send_device_id schedules TX finish immediately
    // (this only happens if not in progress)
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish,
                                            RD_SUCCESS);
    app_uart_on_evt_send_device_id (NULL, 0);
}

void test_app_uart_send_broadcast_ok_coded_phy (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_adv_scan_t scan =
    {
        .addr = MOCK_MAC_ADDR_INIT(),
        .rssi = -51,
        .data = MOCK_DATA_INIT(),
        .data_len = sizeof (mock_data),
        .is_coded_phy = true,
        .primary_phy = RE_CA_UART_BLE_PHY_CODED,
        .secondary_phy = RE_CA_UART_BLE_PHY_CODED,
        .ch_index = 10,
        .tx_power = 8,
    };
    test_app_uart_init_ok();
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_send_broadcast (&scan); // Call the function under test
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_send_broadcast_ok_extended_adv_2m_phy (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_adv_scan_t scan =
    {
        .addr = MOCK_MAC_ADDR_INIT(),
        .rssi = -52,
        .data = MOCK_DATA_INIT(),
        .data_len = sizeof (mock_data),
        .is_coded_phy = false,
        .primary_phy = RE_CA_UART_BLE_PHY_1MBPS,
        .secondary_phy = RE_CA_UART_BLE_PHY_2MBPS,
        .ch_index = 39,
        .tx_power = 0,
    };
    test_app_uart_init_ok();
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_send_broadcast (&scan); // Call the function under test
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// Cover default case in app_uart_on_evt_tx_finish: set an invalid g_resp_type
// and verify that only TX-in-progress flag is cleared (no actions taken),
// which we confirm by the fact that app_uart_on_evt_send_device_id schedules
// tx finish immediately after.
void test_app_uart_on_evt_tx_finish_with_unknown_resp_type_clears_tx_flag (void)
{
    // Reset globals to known state
    app_uart_init_globs();
    // Simulate that a TX was in progress and response type is invalid (-1)
    app_uart_test_set_tx_in_progress (true);
    app_uart_test_set_resp_type (-1);
    // Call the function under test. It should only clear the TX flag and log error.
    app_uart_on_evt_tx_finish (NULL, 0);
    // Now when we request to send device id, scheduler should be invoked because
    // TX is no longer in progress.
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish,
                                            RD_SUCCESS);
    app_uart_on_evt_send_device_id (NULL, 0);
}

void test_app_uart_send_broadcast_ok_phy_auto (void)
{
    rd_status_t err_code = RD_SUCCESS;
    const ri_adv_scan_t scan =
    {
        .addr = MOCK_MAC_ADDR_INIT(),
        .rssi = -53,
        .data = MOCK_DATA_INIT(),
        .data_len = sizeof (mock_data),
        .is_coded_phy = false,
        .primary_phy = RE_CA_UART_BLE_PHY_AUTO,
        .secondary_phy = RE_CA_UART_BLE_PHY_AUTO,
        .ch_index = 40,
        .tx_power = -1,
    };
    test_app_uart_init_ok();
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_send_broadcast (&scan); // Call the function under test
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_send_broadcast_null (void)
{
    rd_status_t err_code = RD_SUCCESS;
    test_app_uart_init_ok();
    err_code |= app_uart_send_broadcast (NULL);
    TEST_ASSERT_EQUAL (RD_ERROR_NULL, err_code);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

void test_app_uart_send_broadcast_encoding_error (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_adv_scan_t scan = {0};
    memcpy (scan.addr, &mock_mac, sizeof (scan.addr));
    scan.rssi = -50;
    memcpy (scan.data, &mock_data, sizeof (mock_data));
    scan.data_len = sizeof (mock_data);
    test_app_uart_init_ok();
    ri_adv_parse_manuid_ExpectAnyArgsAndReturn (mock_manuf_id);
    uint16_t manufacturer_id = 0x0499;
    app_ble_manufacturer_filter_enabled_ExpectAndReturn (&manufacturer_id, true);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_ERROR_INTERNAL);
    err_code |= app_uart_send_broadcast (&scan);
    TEST_ASSERT_EQUAL (RD_ERROR_INVALID_DATA, err_code);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

void test_app_uart_send_broadcast_error_size (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_adv_scan_t scan = {0};
    memcpy (scan.addr, &mock_mac, sizeof (scan.addr));
    scan.rssi = -50;
    memcpy (scan.data, &mock_data, sizeof (mock_data));
    scan.data_len = 255U;
    test_app_uart_init_ok();
    err_code |= app_uart_send_broadcast (&scan);
    TEST_ASSERT_EQUAL (RD_ERROR_DATA_SIZE, err_code);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

/**
 * @brief Poll scanning configuration through UART.
 *
 * The format is defined by ruuvi.endpoints.c/
 *
 * @retval RD_SUCCESS If encoding and queuing data to UART was successful.
 * @retval RD_ERROR_INVALID_DATA If poll cannot be encoded for any reason.
 */
void test_app_uart_poll_configuration_ok (void)
{
    rd_status_t err_code = RD_SUCCESS;
    static ri_uart_init_t config =
    {
        .hwfc_enabled = RB_HWFC_ENABLED,
        .parity_enabled = RB_PARITY_ENABLED,
        .cts  = RB_UART_CTS_PIN,
        .rts  = RB_UART_RTS_PIN,
        .rx   = RB_UART_RX_PIN,
        .tx   = RB_UART_TX_PIN,
        .baud = RI_UART_BAUD_115200 //!< XXX hardcoded, should come from board.
    };
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_uart_init_ReturnThruPtr_channel (&dummy_uart_success);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    err_code = app_uart_init();
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_scheduler_execute_ExpectAndReturn (RD_SUCCESS);
    ri_yield_ExpectAndReturn (RD_SUCCESS);
    err_code |= app_uart_poll_configuration();
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

void test_app_uart_poll_configuration_encoding_error (void)
{
    rd_status_t err_code = RD_SUCCESS;
    test_app_uart_init_ok();
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_ERROR_INTERNAL);
    err_code |= app_uart_poll_configuration();
    TEST_ASSERT_EQUAL (RD_ERROR_INVALID_DATA, err_code);
    TEST_ASSERT_EQUAL (0, mock_sends);
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

void test_app_uart_parser_get_device_id_ok (void)
{
    uint8_t data[] =
    {
        RE_CA_UART_STX,
        0 + CMD_IN_LEN,
        RE_CA_UART_GET_DEVICE_ID,
        0x36U, 0x8EU, //crc
        RE_CA_UART_ETX
    };
    ri_scheduler_event_put_ExpectAndReturn (data, 6, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED,
                  (void *) &data[0], 6);
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_payload_t expect_payload = {.cmd = RE_CA_UART_GET_DEVICE_ID};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data[0],
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    re_ca_uart_decode_ReturnThruPtr_payload ((re_ca_uart_payload_t *) &expect_payload);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_device_id,
                                            RD_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish,
                                            RD_SUCCESS);
    ri_radio_address_get_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_comm_id_get_ExpectAnyArgsAndReturn (RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) &data[0], 6);
    app_uart_on_evt_send_device_id (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_isr_received (void)
{
    rd_status_t err_code = RD_SUCCESS;
    uint8_t data[] =
    {
        RE_CA_UART_STX,
        2 + CMD_IN_LEN,
        RE_CA_UART_SET_CH_37,
        0x01U,
        RE_CA_UART_FIELD_DELIMITER,
        0xB6U, 0x78U, //crc
        RE_CA_UART_ETX
    };
    ri_scheduler_event_put_ExpectAndReturn (data, 8, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    err_code |= app_uart_isr (RI_COMM_RECEIVED,
                              (void *) &data[0], 8);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_isr_sent (void)
{
    rd_status_t err_code = RD_SUCCESS;
    // Expect scheduling of TX finish handler when RI_COMM_SENT occurs
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    err_code |= app_uart_isr (RI_COMM_SENT, NULL, 0);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_isr_unknown (void)
{
    rd_status_t err_code = RD_SUCCESS;
    rd_error_check_ExpectAnyArgs();
    err_code |= app_uart_isr (RI_COMM_TIMEOUT, NULL, 0);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_fltr_tags (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_FLTR_TAGS,
        .params.bool_param.state = 1,
    };
    app_ble_manufacturer_filter_set_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_fltr_id (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_FLTR_ID,
        .params.fltr_id_param.id = 0x101,
    };
    app_ble_manufacturer_id_set_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_coded_phy (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_CODED_PHY,
        .params.bool_param.state = 1,
    };
    app_ble_modulation_enable_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_scan_1mb (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_SCAN_1MB_PHY,
        .params.bool_param.state = 1,
    };
    app_ble_modulation_enable_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_scan_2mb (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_SCAN_2MB_PHY,
        .params.bool_param.state = 1,
    };
    app_ble_modulation_enable_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_ch_37 (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_CH_37,
        .params.bool_param.state = 1,
    };
    app_ble_channels_get_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_ble_channels_set_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_ch_38 (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_CH_38,
        .params.bool_param.state = 1,
    };
    app_ble_channels_get_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_ble_channels_set_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_ch_39 (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_CH_39,
        .params.bool_param.state = 1,
    };
    app_ble_channels_get_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_ble_channels_set_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_all_max_adv_len_zero (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_ALL,
        .params.all_params.fltr_id.id = 0x101,
        .params.all_params.bools.fltr_tags.state = 1,
        .params.all_params.bools.use_coded_phy.state = 0,
        .params.all_params.bools.use_1m_phy.state = 1,
        .params.all_params.bools.use_2m_phy.state = 0,
        .params.all_params.bools.ch_37.state = 1,
        .params.all_params.bools.ch_38.state = 0,
        .params.all_params.bools.ch_39.state = 1,
        .params.all_params.max_adv_len = 0,
    };
    app_ble_manufacturer_id_set_ExpectAndReturn (0x101, RD_SUCCESS);
    app_ble_manufacturer_filter_set_ExpectAndReturn (true, RD_SUCCESS);
    app_ble_set_max_adv_len_Expect (0);
    ri_radio_channels_t channels = { 0 };
    channels.channel_37 = 1;
    channels.channel_38 = 0;
    channels.channel_39 = 1;
    app_ble_channels_set_ExpectAndReturn (channels, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_125KBPS, false, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_1MBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_2MBPS, false, RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}

void test_app_uart_apply_config_all_max_adv_len_non_zero (void)
{
    rd_status_t err_code = RD_SUCCESS;
    re_ca_uart_payload_t payload =
    {
        .cmd = RE_CA_UART_SET_ALL,
        .params.all_params.fltr_id.id = 0x102,
        .params.all_params.bools.fltr_tags.state = 1,
        .params.all_params.bools.use_coded_phy.state = 1,
        .params.all_params.bools.use_1m_phy.state = 0,
        .params.all_params.bools.use_2m_phy.state = 1,
        .params.all_params.bools.ch_37.state = 0,
        .params.all_params.bools.ch_38.state = 1,
        .params.all_params.bools.ch_39.state = 0,
        .params.all_params.max_adv_len = 48,
    };
    app_ble_manufacturer_id_set_ExpectAndReturn (0x102, RD_SUCCESS);
    app_ble_manufacturer_filter_set_ExpectAndReturn (true, RD_SUCCESS);
    app_ble_set_max_adv_len_Expect (48);
    ri_radio_channels_t channels = { 0 };
    channels.channel_37 = 0;
    channels.channel_38 = 1;
    channels.channel_39 = 0;
    app_ble_channels_set_ExpectAndReturn (channels, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_125KBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_1MBPS, false, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_2MBPS, true, RD_SUCCESS);
    err_code |= app_uart_apply_config (&payload);
    TEST_ASSERT_EQUAL (RD_SUCCESS, err_code);
}


void test_app_uart_parser_ok (void)
{
    uint8_t data[] =
    {
        RE_CA_UART_STX,
        2 + CMD_IN_LEN,
        RE_CA_UART_SET_CH_37,
        0x01U,
        RE_CA_UART_FIELD_DELIMITER,
        0xB6U, 0x78U, //crc
        RE_CA_UART_ETX
    };
    ri_scheduler_event_put_ExpectAndReturn (data, 8, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED,
                  (void *) &data[0], 8);
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data[0],
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data, 8);
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_parser_clean_old (void)
{
    uint8_t data[] =
    {
        RE_CA_UART_STX,
        2 + CMD_IN_LEN,
        RE_CA_UART_SET_CH_37,
        0x01U,
        RE_CA_UART_FIELD_DELIMITER,
        0xB6U, 0x78U, //crc
        RE_CA_UART_ETX
    };
    uint8_t * p_data_0 = &data[0];
    ri_scheduler_event_put_ExpectAndReturn (data, 8, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED,
                  (void *) &data[0], 8);
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data[0],
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_0, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data, 8);
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_parser_part_1_ok (void)
{
    uint8_t data_part1[] =
    {
        RE_CA_UART_STX,
        2 + CMD_IN_LEN,
        RE_CA_UART_SET_CH_37,
    };
    uint8_t * p_data_0 = &data_part1[0];
    uint8_t * p_data_1 = &data_part1[1];
    uint8_t * p_data_2 = &data_part1[2];
    ri_scheduler_event_put_ExpectAndReturn (data_part1, 3, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED,
                  (void *) &data_part1[0], 3);
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data_part1[0],
                                       (re_ca_uart_payload_t *) &payload, RE_ERROR_DECODING_CRC);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_0, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_1, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_2, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    re_ca_uart_decode_ExpectAnyArgsAndReturn (RE_ERROR_DECODING_CRC);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data_part1, 3);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

void test_app_uart_parser_part_2_ok (void)
{
    uint8_t data_part1[] =
    {
        0x01U,
        RE_CA_UART_FIELD_DELIMITER,
        0xB6U, 0x78U, //crc
        RE_CA_UART_ETX
    };
    ri_scheduler_event_put_ExpectAndReturn (data_part1, 5, &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED,
                  (void *) &data_part1[0], sizeof (data_part1));
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data_part1[0],
                                       (re_ca_uart_payload_t *) &payload, RE_ERROR_DECODING_CRC);

    for (size_t ii = 0; ii < sizeof (data_part1); ii++)
    {
        rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    }

    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    re_ca_uart_decode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data_part1, 5);
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// New tests to cover branches inside app_uart_send_ack

void test_app_uart_send_ack_is_ok_true (void)
{
    // This test drives the ACK path with is_ok == true via LED_CTRL command
    // which sets g_resp_ack_state = true and schedules ACK send.
    // First, ensure UART is initialized so messages can be sent via mock_uart
    test_app_uart_init_ok();
    // Prepare dummy incoming data buffer and expectations for scheduling parser
    uint8_t data[] = { 0x02, 0x00, 0x00 };
    ri_scheduler_event_put_ExpectAndReturn (data, sizeof (data), &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED, (void *) data, sizeof (data));
    // app_uart_parser should decode a LED_CTRL payload and then:
    // - stop activity LED
    // - blink once (non-zero interval)
    // - schedule ACK send event
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_payload_t expect_payload = {0};
    expect_payload.cmd = RE_CA_UART_LED_CTRL;
    expect_payload.params.led_ctrl_param.time_interval_ms = 10;
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) data,
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    re_ca_uart_decode_ReturnThruPtr_payload ((re_ca_uart_payload_t *) &expect_payload);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    rt_led_blink_stop_ExpectAndReturn (RB_LED_ACTIVITY, RD_SUCCESS);
    rt_led_blink_once_ExpectAndReturn (RB_LED_ACTIVITY, 10, RD_SUCCESS);
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    // When ACK event is handled, it schedules TX finish
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    // Encoding succeeds, so UART send should be called once
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser (data, sizeof (data));
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

void test_app_uart_send_ack_encode_error_false_branch (void)
{
    // Initialize UART
    static ri_uart_init_t config =
    {
        .hwfc_enabled = RB_HWFC_ENABLED,
        .parity_enabled = RB_PARITY_ENABLED,
        .cts  = RB_UART_CTS_PIN,
        .rts  = RB_UART_RTS_PIN,
        .tx   = RB_UART_TX_PIN,
        .rx   = RB_UART_RX_PIN,
        .baud = RI_UART_BAUD_115200
    };
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_uart_init_ReturnThruPtr_channel (&mock_uart);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    TEST_ASSERT_EQUAL (RD_SUCCESS, app_uart_init());
    // Directly request to send ACK (g_resp_ack_state default is false after init)
    // app_uart_on_evt_send_ack should schedule TX finish
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    // Force encode error so (RE_SUCCESS == err_code) is false and no UART send happens
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_ERROR_INTERNAL);
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    // Verify that no data was sent over UART
    TEST_ASSERT_EQUAL (0, mock_sends);
}

// Cover LED_CTRL branch in app_uart_parser when time_interval_ms == 0
void test_app_uart_parser_led_ctrl_zero_interval (void)
{
    // Drive parser directly with a decoded LED_CTRL payload where interval == 0
    uint8_t data[] = { 0xAA, 0xBB, 0xCC }; // dummy buffer, not used by decode stub
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_payload_t expect_payload = {0};
    expect_payload.cmd = RE_CA_UART_LED_CTRL;
    expect_payload.params.led_ctrl_param.time_interval_ms = 0;
    // app_uart_parser first tries to decode and then drains ringbuffer
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) data,
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    re_ca_uart_decode_ReturnThruPtr_payload ((re_ca_uart_payload_t *) &expect_payload);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    // LED activity should be stopped, and no blink_once because interval == 0
    rt_led_blink_stop_ExpectAndReturn (RB_LED_ACTIVITY, RD_SUCCESS);
    // ACK event should be scheduled
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    // When ACK is handled, TX finish is scheduled and encoding succeeds -> one send
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser (data, sizeof (data));
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// Cover RE_CA_UART_SET_ALL branch in app_uart_parser: triggers scan start and watchdog feed
void test_app_uart_parser_set_all_triggers_scan_start (void)
{
    // Prepare ISR to schedule parser
    uint8_t data[] = { RE_CA_UART_STX, 0x00, 0x00 }; // dummy; content not used beyond decode stub
    ri_scheduler_event_put_ExpectAndReturn (data, sizeof (data), &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED, (void *) data, sizeof (data));
    // Prepare decoded payload for SET_ALL that results in successful apply_config
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_payload_t expect_payload = {0};
    expect_payload.cmd = RE_CA_UART_SET_ALL;
    expect_payload.params.all_params.fltr_id.id = 0x1234;
    expect_payload.params.all_params.bools.fltr_tags.state = 1;
    expect_payload.params.all_params.bools.use_coded_phy.state = 1;
    expect_payload.params.all_params.bools.use_1m_phy.state = 1;
    expect_payload.params.all_params.bools.use_2m_phy.state = 0;
    expect_payload.params.all_params.bools.ch_37.state = 1;
    expect_payload.params.all_params.bools.ch_38.state = 0;
    expect_payload.params.all_params.bools.ch_39.state = 1;
    expect_payload.params.all_params.max_adv_len = 16;
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) data,
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    re_ca_uart_decode_ReturnThruPtr_payload ((re_ca_uart_payload_t *) &expect_payload);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    // Expectations for app_uart_apply_config called inside parser for SET_ALL
    app_ble_manufacturer_id_set_ExpectAndReturn (0x1234, RD_SUCCESS);
    app_ble_manufacturer_filter_set_ExpectAndReturn (true, RD_SUCCESS);
    app_ble_set_max_adv_len_Expect (16);
    ri_radio_channels_t channels = {0};
    channels.channel_37 = 1;
    channels.channel_38 = 0;
    channels.channel_39 = 1;
    app_ble_channels_set_ExpectAndReturn (channels, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_125KBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_1MBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_2MBPS, false, RD_SUCCESS);
    // ACK event will be scheduled
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    // After SET_ALL, scan should start and watchdog fed on success
    app_ble_scan_start_ExpectAndReturn (RD_SUCCESS);
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    // ACK handling and encode/send flow
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser (data, sizeof (data));
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// Cover the else branch after err_code |= app_ble_scan_start():
// When scan_start returns an error, watchdog must NOT be fed.
void test_app_uart_parser_set_all_scan_start_error_no_watchdog (void)
{
    // Prepare decoded payload for SET_ALL that results in successful apply_config
    uint8_t data[] = { RE_CA_UART_STX, 0x00, 0x00 }; // dummy; content not used beyond decode stub
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_payload_t expect_payload = {0};
    expect_payload.cmd = RE_CA_UART_SET_ALL;
    expect_payload.params.all_params.fltr_id.id = 0x2222;
    expect_payload.params.all_params.bools.fltr_tags.state = 1;
    expect_payload.params.all_params.bools.use_coded_phy.state = 0;
    expect_payload.params.all_params.bools.use_1m_phy.state = 1;
    expect_payload.params.all_params.bools.use_2m_phy.state = 1;
    expect_payload.params.all_params.bools.ch_37.state = 1;
    expect_payload.params.all_params.bools.ch_38.state = 1;
    expect_payload.params.all_params.bools.ch_39.state = 1;
    expect_payload.params.all_params.max_adv_len = 31;
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) data,
                                       (re_ca_uart_payload_t *) &payload, RD_SUCCESS);
    re_ca_uart_decode_ReturnThruPtr_payload ((re_ca_uart_payload_t *) &expect_payload);
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    // Expectations for app_uart_apply_config called inside parser for SET_ALL
    app_ble_manufacturer_id_set_ExpectAndReturn (0x2222, RD_SUCCESS);
    app_ble_manufacturer_filter_set_ExpectAndReturn (true, RD_SUCCESS);
    app_ble_set_max_adv_len_Expect (31);
    ri_radio_channels_t channels = {0};
    channels.channel_37 = 1;
    channels.channel_38 = 1;
    channels.channel_39 = 1;
    app_ble_channels_set_ExpectAndReturn (channels, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_125KBPS, false, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_1MBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_2MBPS, true, RD_SUCCESS);
    // ACK event will be scheduled regardless of scan_start result
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_send_ack, RD_SUCCESS);
    // After SET_ALL, scan should start but fail; watchdog must NOT be called
    app_ble_scan_start_ExpectAndReturn (RD_ERROR_INVALID_STATE);
    // Do not set any expectation for ri_watchdog_feed — a call would fail the test
    // ACK handling and encode/send flow
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    app_uart_parser (data, sizeof (data));
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    // Verify one UART send happened for ACK
    TEST_ASSERT_EQUAL (1, mock_sends);
}

// Additional coverage: exercise both termination conditions of do-while loops
// after rl_ringbuffer_queue() inside app_uart_parser.
// Case 1: First queue loop exits due to non-success status, and the re-queue
// loop (after failed re-decode) also exits immediately due to non-success
// with len == 0 (do-while executes once).
void test_app_uart_parser_initial_queue_failure_and_requeue_zero_len_failure (void)
{
    uint8_t data[] = { 0x11, 0x22, 0x33 };
    // ISR schedules parser
    ri_scheduler_event_put_ExpectAndReturn (data, sizeof (data), &app_uart_parser,
                                            RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED, (void *) data, sizeof (data));
    // Initial decode fails -> enters first queue loop
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data[0],
                                       (re_ca_uart_payload_t *) &payload,
                                       RE_ERROR_DECODING_CRC);
    // First queue attempt fails -> loop exits by status != RL_SUCCESS
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_ERROR_NO_MEM);
    // Dequeue finds no data
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    // Decode of empty dequeue buffer fails again
    re_ca_uart_decode_ExpectAnyArgsAndReturn (RE_ERROR_DECODING_CRC);
    // Re-queue loop with len == 0 still executes body once; force failure exit
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_ERROR_NO_MEM);
    // No sends expected
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data, (uint16_t) sizeof (data));
    TEST_ASSERT_EQUAL (0, mock_sends);
}

// Case 2: First queue loop exits by reaching index == data_len (all successes),
// then re-decode fails and the second queue loop exits due to non-success status.
void test_app_uart_parser_requeue_loop_exit_by_failure (void)
{
    uint8_t data_part1[] =
    {
        RE_CA_UART_STX,
        (uint8_t) (2 + CMD_IN_LEN),
        RE_CA_UART_SET_CH_37,
    };
    uint8_t * p_data_0 = &data_part1[0];
    uint8_t * p_data_1 = &data_part1[1];
    uint8_t * p_data_2 = &data_part1[2];
    // Schedule parser via ISR
    ri_scheduler_event_put_ExpectAndReturn (data_part1, 3, &app_uart_parser, RD_SUCCESS);
    rd_error_check_ExpectAnyArgs();
    app_uart_isr (RI_COMM_RECEIVED, (void *) &data_part1[0], 3);
    // Initial decode fails
    re_ca_uart_payload_t payload = {0};
    re_ca_uart_decode_ExpectAndReturn ((uint8_t *) &data_part1[0],
                                       (re_ca_uart_payload_t *) &payload, RE_ERROR_DECODING_CRC);
    // First queue loop succeeds for all bytes -> exit by index < data_len becoming false
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    // Dequeue previously queued bytes
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_0, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_1, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_dequeue_ReturnMemThruPtr_data (&p_data_2, sizeof (uint8_t *));
    rl_ringbuffer_dequeue_ExpectAnyArgsAndReturn (RL_ERROR_NO_DATA);
    // Re-decode fails
    re_ca_uart_decode_ExpectAnyArgsAndReturn (RE_ERROR_DECODING_CRC);
    // Second queue loop: first success, then failure -> exit by status
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_SUCCESS);
    rl_ringbuffer_queue_ExpectAnyArgsAndReturn (RL_ERROR_NO_MEM);
    // No sends expected
    ri_watchdog_feed_IgnoreAndReturn (RD_SUCCESS);
    app_uart_parser ((void *) data_part1, 3);
    TEST_ASSERT_EQUAL (0, mock_sends);
}

// app_uart_on_evt_send_ack: cover false branch of `if (!g_flag_uart_tx_in_progress)`
// When TX is already in progress, the function must NOT schedule TX finish event.
void test_app_uart_on_evt_send_ack_when_tx_in_progress_does_not_schedule (void)
{
    // Initialize UART to use mock_uart (which increments mock_sends on send)
    static ri_uart_init_t config =
    {
        .hwfc_enabled = RB_HWFC_ENABLED,
        .parity_enabled = RB_PARITY_ENABLED,
        .cts  = RB_UART_CTS_PIN,
        .rts  = RB_UART_RTS_PIN,
        .tx   = RB_UART_TX_PIN,
        .rx   = RB_UART_RX_PIN,
        .baud = RI_UART_BAUD_115200
    };
    ri_uart_init_ExpectAnyArgsAndReturn (RD_SUCCESS);
    ri_uart_init_ReturnThruPtr_channel (&mock_uart);
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    TEST_ASSERT_EQUAL (RD_SUCCESS, app_uart_init());
    // First ACK request when TX is not in progress should schedule TX finish
    ri_scheduler_event_put_ExpectAndReturn (NULL, 0, &app_uart_on_evt_tx_finish, RD_SUCCESS);
    // Encoding succeeds so the ACK will be sent during tx_finish handler
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    // Request to send ACK and handle TX finish, which triggers actual UART send
    app_uart_on_evt_send_ack (NULL, 0);
    app_uart_on_evt_tx_finish (NULL, 0);
    // Verify one UART send happened and TX is now in progress
    TEST_ASSERT_EQUAL (1, mock_sends);
    // Now request to send ACK again while TX is in progress — must NOT schedule TX finish.
    // We set no expectation for ri_scheduler_event_put here; any call would fail the test.
    app_uart_on_evt_send_ack (NULL, 0);
}
