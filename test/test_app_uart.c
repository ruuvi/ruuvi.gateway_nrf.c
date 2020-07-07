#include "unity.h"

#include "app_uart.h"

#include "ruuvi_boards.h"
#include "ruuvi_interface_communication_ble_advertising.h"

#include "mock_ruuvi_endpoint_ca_uart.h"
#include "mock_ruuvi_interface_communication_uart.h"

#include <string.h>

/** https://stackoverflow.com/questions/3385515/static-assert-in-c, Nordic Mainframe */
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]

/** https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c Joey Adams*/
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

STATIC_ASSERT (MEMBER_SIZE (ri_adv_scan_t, addr) == MEMBER_SIZE (re_ca_uart_ble_adv_t,
               mac), \
               Size_mismatch);

const uint8_t mock_mac[BLE_MAC_ADDRESS_LENGTH] = {0xFA, 0xEB, 0xDC, 0xCD, 0xBE, 0xAF};
const uint8_t mock_data[] =
{
    0x05U, 0x0FU, 0x27U, 0x40U,
    0x35U, 0xC4U, 0x54U, 0x54U, 0x50U, 0x00U, 0xC8U, 0xFCU,
    0x20U, 0xA4U, 0x56U, 0xF0U, 0x30U, 0xE5U, 0xC9U, 0x44U, 0x54U, 0x29U, 0xE3U,
    0x8DU
};

static size_t mock_sends = 0;
// Mock sending fp for data through uart.
static rd_status_t mock_send (ri_comm_message_t * const msg)
{
    mock_sends++;
    return RD_SUCCESS;
}



static ri_comm_channel_t mock_uart =
{
    .send = &mock_send
};


void setUp (void)
{
    mock_sends = 0;
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
    ri_uart_init_t config =
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
    rd_status_t err_code = app_uart_init ();
    TEST_ASSERT (RD_SUCCESS == err_code);
}
void test_app_uart_init_twice (void)
{
    ri_uart_init_t config =
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
    ri_uart_config_ExpectWithArrayAndReturn (&config, 1, RD_SUCCESS);
    rd_status_t err_code = app_uart_init ();
    ri_uart_init_ExpectAnyArgsAndReturn (RD_ERROR_INVALID_STATE);
    err_code |= app_uart_init ();
    TEST_ASSERT (RD_ERROR_INVALID_STATE == err_code);
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
void test_app_uart_send_broadcast_ok (void)
{
    rd_status_t err_code = RD_SUCCESS;
    ri_adv_scan_t scan = {0};
    memcpy (scan.addr, &mock_mac, sizeof (scan.addr));
    scan.rssi = -50;
    memcpy (scan.data, &mock_data, sizeof (mock_data));
    scan.data_len = sizeof (mock_data);
    test_app_uart_init_ok();
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_SUCCESS);
    err_code |= app_uart_send_broadcast (&scan);
    TEST_ASSERT (RD_SUCCESS == err_code);
    TEST_ASSERT (1 == mock_sends);
}

void test_app_uart_send_broadcast_null (void)
{
    rd_status_t err_code = RD_SUCCESS;
    test_app_uart_init_ok();
    err_code |= app_uart_send_broadcast (NULL);
    TEST_ASSERT (RD_ERROR_NULL == err_code);
    TEST_ASSERT (0 == mock_sends);
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
    re_ca_uart_encode_ExpectAnyArgsAndReturn (RD_ERROR_INTERNAL);
    err_code |= app_uart_send_broadcast (&scan);
    TEST_ASSERT (RD_ERROR_INVALID_DATA == err_code);
    TEST_ASSERT (0 == mock_sends);
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
    TEST_ASSERT (RD_ERROR_DATA_SIZE == err_code);
    TEST_ASSERT (0 == mock_sends);
}