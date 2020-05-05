#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_util.h"
#include "nrf_sdh_soc.h"
#include "nrf_pwr_mgmt.h"
#include "ble.h"
#include "uart.h"
#include "app_scheduler.h"
#include "nrf_drv_wdt.h"
#include "nrf_gpio.h"

#define NRF_LOG_MODULE_NAME dongle
#define NRF_LOG_LEVEL 4
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

struct adv_broadcast
{
    char buf[10];
    int len;
};

nrf_drv_wdt_channel_id m_channel_id;
APP_TIMER_DEF (wdt_timer);
APP_TIMER_DEF (led_timer);

#define LED_PIN 29
#define LED_BLINK_DURATION 100

#define SCHED_QUEUE_SIZE 16
#define SCHED_EVENT_DATA_SIZE 200

// error_info_t error_info = {
//     .line_num    = line_num,
//     .p_file_name = p_file_name,
//     .err_code    = error_code,
// };
// app_error_fault_handler(NRF_FAULT_ID_SDK_ERROR, 0, (uint32_t)(&error_info));

void app_error_fault_handler (uint32_t id, uint32_t pc, uint32_t info)
{
    error_info_t * err = (error_info_t *) info;
    NRF_LOG_ERROR ("err: %d - %s,line: %d, file: %s\n", err->err_code,
                   NRF_LOG_ERROR_STRING_GET (err->err_code), err->line_num, (uint32_t) err->p_file_name);
    NRF_LOG_FINAL_FLUSH();
    // On assert, the system can only recover with a reset.\r
#ifndef DEBUG
    NVIC_SystemReset();
#else
    app_error_save_and_stop (id, pc, info);
#endif // DEBUG
}
/**@brief Function for handling asserts in the SoftDevice.
 *
 * @details This function is called in case of an assert in the SoftDevice.
 *
 * @warning This handler is only an example and is not meant for the final product. You need to analyze
 *          how your product is supposed to react in case of assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing assert call.
 * @param[in] p_file_name  File name of the failing assert call.
 */
/* void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%d, %s", line_num, p_file_name);
    NRF_LOG_FLUSH();
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
} */

void scheduler_init()
{
    APP_SCHED_INIT (SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/**@brief Function for initializing the timer. */
static void timer_init (void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK (err_code);
}

/**@brief Function for initializing the nrf log module. */
static void log_init (void)
{
    ret_code_t err_code = NRF_LOG_INIT (app_timer_cnt_get);
    APP_ERROR_CHECK (err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init (void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK (err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details Handles any pending log operations, then sleeps until the next event occurs.
 */
static void idle_state_handle (void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

static void led_timer_handler (void * p_context)
{
    //NRF_LOG_DEBUG("%s", __func__);
    nrf_gpio_pin_clear (LED_PIN);
}

void led_blink()
{
    nrf_gpio_pin_set (LED_PIN);
    app_timer_start (led_timer, APP_TIMER_TICKS (LED_BLINK_DURATION), 0);
}

static void led_init()
{
    nrf_gpio_cfg_output (LED_PIN);
    nrf_gpio_pin_clear (LED_PIN);
    app_timer_create (&led_timer, APP_TIMER_MODE_SINGLE_SHOT, led_timer_handler);
}

static void wdt_event_handler()
{
    NRF_LOG_INFO ("%s", __func__);
    NRF_LOG_FLUSH();
}

static void wdt_timer_handler (void * p_context)
{
    //NRF_LOG_INFO("%s", __func__);
    nrf_drv_wdt_channel_feed (m_channel_id);
}

static void wdt_init (void)
{
    ret_code_t err_code;
    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
    err_code = nrf_drv_wdt_init (&config, wdt_event_handler);
    APP_ERROR_CHECK (err_code);
    err_code = nrf_drv_wdt_channel_alloc (&m_channel_id);
    APP_ERROR_CHECK (err_code);
    nrf_drv_wdt_enable();
    err_code = app_timer_create (&wdt_timer, APP_TIMER_MODE_REPEATED, wdt_timer_handler);
    APP_ERROR_CHECK (err_code);
    err_code = app_timer_start (wdt_timer, APP_TIMER_TICKS (2000), 0);
    APP_ERROR_CHECK (err_code);
}

int main (void)
{
    // Initialize.
    timer_init();
    scheduler_init();
    log_init();
    wdt_init();
    uart_init();
    led_init();
    power_management_init();
    ble_stack_init();
    scan_init();
    // Start execution.
    NRF_LOG_INFO ("Ruuvi Dongle started.");
    scan_start();

    // Enter main loop.
    for (;;)
    {
        app_sched_execute();
        idle_state_handle();
    }
}
