/**
 *  @file main.c
 *  @author Otso Jousimaa <otso@ojousima.net>
 *  @date 2020-05-12
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  Application main.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_gpio.h"
#include "ruuvi_interface_log.h"
#include "ruuvi_interface_scheduler.h"
#include "ruuvi_interface_timer.h"
#include "ruuvi_interface_watchdog.h"
#include "ruuvi_interface_yield.h"
#include "ruuvi_task_led.h"
#include "ruuvi_endpoint_ca_uart.h"
#include "main.h"
#include "app_ble.h"
#include "app_uart.h"
#if !defined(CEEDLING) && !defined(SONAR)
#include "nrf_log.h"
#else
#define NRF_LOG_INFO(fmt, ...)
#define NRF_LOG_ERROR(fmt, ...)
#endif

#define LED_ON_TIME_AFTER_REBOOT_MS (4000U)  //!< Turn on LED for 4 seconds after reboot

/**
 * @brief Convert MAC address to string.
 *
 * @param[in] p_mac Pointer to array of 6 bytes with MAC address.
 *
 * @return MAC address as string.
 */
mac_addr_str_t mac_addr_to_str (const uint8_t * const p_mac)
{
    mac_addr_str_t mac_addr_str = {0};
#if RI_LOG_ENABLED
    snprintf (mac_addr_str.buf, sizeof (mac_addr_str.buf),
              "%02x:%02x:%02x:%02x:%02x:%02x",
              p_mac[0], p_mac[1], p_mac[2], p_mac[3], p_mac[4], p_mac[5]);
#endif
    return mac_addr_str;
}

/**
 * @brief Configure LEDs as outputs, turn them off.
 */
static rd_status_t leds_init (void)
{
    rd_status_t err_code = RD_SUCCESS;
    static const ri_gpio_id_t led_list[] = RB_LEDS_LIST;
    static const ri_gpio_state_t leds_active[] = RB_LEDS_ACTIVE_STATE;
    err_code |= rt_led_init (led_list, leds_active, sizeof (led_list) / sizeof (led_list[0]));
    err_code |= rt_led_blink_once (RB_LED_ACTIVITY, LED_ON_TIME_AFTER_REBOOT_MS);
    return err_code;
}

#ifndef CEEDLING
static
#endif
void on_wdt (void)
{
    // No action required
}

static void setup (void)
{
    rd_status_t err_code = RD_SUCCESS;
    err_code |= ri_log_init (APP_LOG_LEVEL);
    ri_log (RI_LOG_LEVEL_INFO, "Log initialized\n");
    NRF_LOG_INFO ("RI_COMM_BLE_PAYLOAD_MAX_LENGTH=%d", RI_COMM_BLE_PAYLOAD_MAX_LENGTH);
    NRF_LOG_INFO ("RE_CA_UART_ADV_BYTES=%d", RE_CA_UART_ADV_BYTES);
    NRF_LOG_INFO ("RUUVI_NRF5_SDK15_ADV_ENABLED: %d", RUUVI_NRF5_SDK15_ADV_ENABLED);
    NRF_LOG_INFO ("RUUVI_NRF5_SDK15_ADV_EXTENDED_ENABLED: %d",
                  RUUVI_NRF5_SDK15_ADV_EXTENDED_ENABLED);
    NRF_LOG_INFO ("RUUVI_COMM_BLE_ADV_MAX_LENGTH: %d", RUUVI_COMM_BLE_ADV_MAX_LENGTH);
    NRF_LOG_INFO ("RUUVI_COMM_BLE_ADV_SCAN_LENGTH: %d", RUUVI_COMM_BLE_ADV_SCAN_LENGTH);
    NRF_LOG_INFO ("RUUVI_COMM_BLE_ADV_SCAN_BUFFER: %d", RUUVI_COMM_BLE_ADV_SCAN_BUFFER);
    err_code |= ri_watchdog_init (APP_WDT_INTERVAL_MS, on_wdt);
    err_code |= ri_yield_init();
    err_code |= ri_timer_init();
    err_code |= ri_scheduler_init();
    err_code |= ri_gpio_init();
    // Requires GPIO
    err_code |= leds_init();
    // Requires timers
    err_code |= ri_yield_low_power_enable (true);
    // Requires LEDs
    err_code |= app_uart_init();
#if defined(RUUVI_GW_NRF_POLL_CONFIG_DISABLED) && RUUVI_GW_NRF_POLL_CONFIG_DISABLED
    // Set default configuration by updating macro `RB_BLE_DEFAULT_...` in app_ble.c
    NRF_LOG_INFO ("Use default settings as the initial configuration");
#else
    NRF_LOG_INFO ("Waiting for receiving the initial configuration via UART");
    err_code |= app_uart_poll_configuration();
#endif
    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);
}

/**
 * @brief Application main
 *
 * @retval 0 After unit test run. Does not return when running on embedded target.
 */
#ifdef CEEDLING
int app_main (void)
#else
int main (void)
#endif
{
    rd_status_t err_code = RD_SUCCESS;
    setup();
    err_code |= app_ble_scan_start();
    err_code |= ri_watchdog_feed();
    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);

    do
    {
        ri_scheduler_execute();
        ri_yield();
    } while (LOOP_FOREVER);

    return -1; // Unreachable code unless running unit tests.
}
