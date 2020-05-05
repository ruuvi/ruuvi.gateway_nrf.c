#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nrf_ble_scan.h"
#include "ble_advdata.h"
#include "nrf_ble_gatt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "uart.h"
#include "dongle.h"

#define NRF_LOG_MODULE_NAME ble
#define NRF_LOG_LEVEL 4
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

#define APP_BLE_CONN_CFG_TAG    1                                       /**< Tag that refers to the BLE stack configuration set with @ref sd_ble_cfg_set. The default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */
#define APP_BLE_OBSERVER_PRIO   3

#define RUUVI_COMPANY_ID        0x0499
#define RUUVI_SCAN_PERIOD       6500

NRF_BLE_SCAN_DEF (
    m_scan);                                              /**< Scanning Module instance. */

struct adv_filter
{
    bool active;
    uint16_t company_id;
};
struct adv_filter m_adv_filter;

static ble_gap_scan_params_t m_scan_param;
uint8_t channel_i = 0;
const uint8_t ch_mask_disable = 0xe0;

/* BLE channels to be scanned  */
const uint8_t channels[] =
{
    37,
    38,
    39
};

void set_company_id (uint16_t com_id)
{
    NRF_LOG_INFO ("Setting company_id filter to: 0x%02x%02x", com_id >> 8, com_id & 0xFF);
    m_adv_filter.active = true;
    m_adv_filter.company_id = com_id;
}

void clear_adv_filter()
{
    m_adv_filter.active = false;
}

/**@brief Function for starting scanning. */
void scan_start (void)
{
    NRF_LOG_DEBUG ("Scan start");
    ret_code_t ret;
    ret = nrf_ble_scan_start (&m_scan);
    APP_ERROR_CHECK (ret);
}

/* Returns true if advertisement's company id matches the filter or if filter is disabled  */
static bool filter_company_id (uint8_t * adv_data, uint8_t len)
{
    if (m_adv_filter.active)
    {
        uint8_t * manuf_data = ble_advdata_parse (adv_data, len,
                               BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA);

        if (manuf_data)
        {
            uint16_t company_id;
            memcpy (&company_id, manuf_data, 2);
            return (company_id == m_adv_filter.company_id);
        }
        else
        {
            return false; //no manuf data
        }
    }
    else
    {
        return true; //filter disabled, return all advertisements
    }
}

static void set_scan_channel (uint8_t channel)
{
    if (channel >= 0 && channel <= 39)
    {
        m_scan_param.channel_mask[4] = ch_mask_disable;
        uint8_t b = channel / 8;
        uint8_t bit = channel % 8;
        uint8_t mask = ~ (1 << bit); //set wanted bit to 0
        m_scan_param.channel_mask[b] &= mask;
        NRF_LOG_DEBUG ("Setting scanning channel: ch: %d, b: %d, bit: %d, mask: 0x%x", channel, b,
                       bit, mask);
    }
    else
    {
        NRF_LOG_WARNING ("Invalid channel: %d", channel);
    }
}

/**@brief Function for handling Scanning Module events.
 */
static void scan_evt_handler (scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;

    switch (p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_NOT_FOUND:
        {
            ble_gap_evt_adv_report_t const * p_adv_report = p_scan_evt->params.p_not_found;

            if (filter_company_id (p_adv_report->data.p_data, p_adv_report->data.len))
            {
                uart_send_broadcast (p_adv_report->peer_addr.addr, p_adv_report->data.p_data,
                                     p_adv_report->data.len, p_adv_report->rssi);
                led_blink();
            }
        }
        break;

        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
        {
            NRF_LOG_INFO ("NRF_BLE_SCAN_EVT_FILTER_MATCH");
        }
        break;

        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
        {
            err_code = p_scan_evt->params.connecting_err.err_code;
            APP_ERROR_CHECK (err_code);
        }
        break;

        case NRF_BLE_SCAN_EVT_CONNECTED:
        {
            ble_gap_evt_connected_t const * p_connected =
                p_scan_evt->params.connected.p_connected;
            // Scan is automatically stopped by the connection.
            NRF_LOG_INFO ("Connecting to target %02x%02x%02x%02x%02x%02x",
                          p_connected->peer_addr.addr[0],
                          p_connected->peer_addr.addr[1],
                          p_connected->peer_addr.addr[2],
                          p_connected->peer_addr.addr[3],
                          p_connected->peer_addr.addr[4],
                          p_connected->peer_addr.addr[5]
                         );
        }
        break;

        case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        {
            static uint8_t ch_i = 0;
            uint8_t ch = channels[ (++ch_i) % (sizeof (channels) / sizeof (channels[0]))];
            set_scan_channel (ch);
            scan_start();
        }
        break;

        default:
            break;
    }
}

/**@brief Function for initializing the scanning and setting the filters.
 */
void scan_init (void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;
    memset (&init_scan, 0, sizeof (init_scan));
    memset (&m_scan_param, 0, sizeof (m_scan_param));
    uint16_t interval = (int) (RUUVI_SCAN_PERIOD / 0.625);
    NRF_LOG_INFO ("interval: %d", interval);
    m_scan_param.extended = 0;
    m_scan_param.report_incomplete_evts = 0;
    m_scan_param.active = 0;
    m_scan_param.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
    m_scan_param.scan_phys = BLE_GAP_PHY_1MBPS;
    m_scan_param.interval = interval;
    m_scan_param.window = interval;
    m_scan_param.timeout = RUUVI_SCAN_PERIOD / 10; //in 10 ms units
    NRF_LOG_INFO ("channel_mask:");
    NRF_LOG_HEXDUMP_INFO (m_scan_param.channel_mask, 5);
    memset (m_scan_param.channel_mask, 0,
            sizeof (m_scan_param.channel_mask)); //enable all primary and secondary channels
    m_scan_param.channel_mask[4] = ch_mask_disable; //disable primary channels 37, 38, 39
    NRF_LOG_HEXDUMP_INFO (m_scan_param.channel_mask, 5);
    set_scan_channel (channels[0]); //start with channel 37
    init_scan.connect_if_match = false;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;
    init_scan.p_scan_param = &m_scan_param;
    init_scan.p_conn_param = NULL;
    err_code = nrf_ble_scan_init (&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK (err_code);
    set_company_id (RUUVI_COMPANY_ID);
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler (ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t            err_code = 0;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO ("Disconnected. conn_handle: 0x%x, reason: 0x%x",
                          p_gap_evt->conn_handle,
                          p_gap_evt->params.disconnected.reason);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_INFO ("Connection Request timed out.");
            }

            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported.
            err_code = sd_ble_gap_sec_params_reply (p_ble_evt->evt.gap_evt.conn_handle,
                                                    BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK (err_code);
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update (p_gap_evt->conn_handle,
                       &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK (err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG ("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update (p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK (err_code);
        }
        break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG ("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect (p_ble_evt->evt.gattc_evt.conn_handle,
                                              BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK (err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG ("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect (p_ble_evt->evt.gatts_evt.conn_handle,
                                              BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK (err_code);
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init (void)
{
    ret_code_t err_code;
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK (err_code);
    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set (APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK (err_code);
    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable (&ram_start);
    APP_ERROR_CHECK (err_code);
    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER (m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}
