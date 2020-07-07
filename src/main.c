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
#include "main.h"
#include "app_ble.h"
#include "app_uart.h"
#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication_ble_advertising.h"
#include "ruuvi_interface_gpio.h"
#include "ruuvi_interface_log.h"
#include "ruuvi_interface_scheduler.h"
#include "ruuvi_interface_timer.h"
#include "ruuvi_interface_watchdog.h"
#include "ruuvi_interface_yield.h"
#include "ruuvi_interface_communication_radio.h"
#include "ruuvi_task_led.h"

/**
 * @brief Configure LEDs as outputs, turn them off.
 */
static rd_status_t leds_init (void)
{
    rd_status_t err_code = RD_SUCCESS;
    static const ri_gpio_id_t led_list[] = RB_LEDS_LIST;
    static const ri_gpio_state_t leds_active[] = RB_LEDS_ACTIVE_STATE;
    err_code |= rt_led_init (led_list, leds_active, sizeof (led_list) / sizeof (led_list[0]));
    err_code |= rt_led_activity_led_set (RB_LED_ACTIVITY);
    return err_code;
}

#ifndef CEEDLING
static
#endif
void on_wdt (void)
{
    // No action required
}

static void modulations_setup (void)
{
    if (ri_radio_supports (RI_RADIO_BLE_125KBPS))
    {
        app_ble_modulation_enable (RI_RADIO_BLE_125KBPS, true);
    }

    // 2Mbit/s scans primary advertisements at 1 Mbit/s, and
    // secondary advertisements are scanned on all support PHYs
    // Therefore 1 Mbit/s scanning should be enabled only if
    // 2 Mbit/s is not enabled.
    // app_ble_modulation_enable(RI_RADIO_BLE_1MBPS, true);
    app_ble_modulation_enable (RI_RADIO_BLE_2MBPS, true);
}

static void setup (void)
{
    rd_status_t err_code = RD_SUCCESS;
    err_code |= ri_log_init (APP_LOG_LEVEL);
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
    ri_yield_indication_set (&rt_led_activity_indicate);
    modulations_setup();
    err_code |= app_uart_init();
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
    RD_ERROR_CHECK (err_code, ~RD_ERROR_FATAL);

    do
    {
        ri_scheduler_execute();
        ri_yield();
    } while (LOOP_FOREVER);

    return -1; // Unreachable code unless running unit tests.
}
