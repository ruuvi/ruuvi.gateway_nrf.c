#include "unity.h"

#include "app_config.h"
#include "main.h"
#include "ruuvi_boards.h"

#include "mock_app_ble.h"
#include "mock_app_uart.h"
#include "mock_ruuvi_driver_error.h"
#include "mock_ruuvi_interface_communication_ble_advertising.h"
#include "mock_ruuvi_interface_gpio.h"
#include "mock_ruuvi_interface_log.h"
#include "mock_ruuvi_interface_scheduler.h"
#include "mock_ruuvi_interface_timer.h"
#include "mock_ruuvi_interface_watchdog.h"
#include "mock_ruuvi_interface_yield.h"
#include "mock_ruuvi_interface_communication_radio.h"
#include "mock_ruuvi_task_led.h"

static const ri_gpio_state_t led = 17U;

void setUp (void)
{
    ri_log_Ignore();
    rd_error_check_Ignore();
}

void tearDown (void)
{
}

static void leds_expect (void)
{
    static const ri_gpio_id_t led_list[] = RB_LEDS_LIST;
    static const ri_gpio_state_t leds_active[] = RB_LEDS_ACTIVE_STATE;
    rt_led_init_ExpectWithArrayAndReturn (led_list,
                                          sizeof (led_list) / sizeof (led_list[0]),
                                          leds_active,
                                          sizeof (leds_active) / sizeof (leds_active[0]),
                                          sizeof (led_list) / sizeof (led_list[0]),
                                          RD_SUCCESS);
    rt_led_write_ExpectAndReturn (led, true, RD_SUCCESS);
}

// TODO: Test on nRF52832 boards
void test_app_main (void)
{
    ri_log_init_ExpectAndReturn (APP_LOG_LEVEL, RD_SUCCESS);
    ri_watchdog_init_ExpectAndReturn (APP_WDT_INTERVAL_MS, &on_wdt, RD_SUCCESS);
    ri_yield_init_ExpectAndReturn (RD_SUCCESS);
    ri_timer_init_ExpectAndReturn (RD_SUCCESS);
    ri_scheduler_init_ExpectAndReturn (RD_SUCCESS);
    ri_gpio_init_ExpectAndReturn (RD_SUCCESS);
    leds_expect();
    ri_yield_low_power_enable_ExpectAndReturn (true, RD_SUCCESS);
    ri_radio_supports_ExpectAndReturn (RI_RADIO_BLE_125KBPS, true);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_125KBPS, true, RD_SUCCESS);
    app_ble_modulation_enable_ExpectAndReturn (RI_RADIO_BLE_2MBPS, true, RD_SUCCESS);
    app_uart_init_ExpectAndReturn (RD_SUCCESS);
    app_uart_poll_configuration_ExpectAndReturn (RD_SUCCESS);
    app_ble_scan_start_ExpectAndReturn (RD_SUCCESS);
    ri_scheduler_execute_ExpectAndReturn (RD_SUCCESS);
    ri_yield_ExpectAndReturn (RD_SUCCESS);
    app_main();
}