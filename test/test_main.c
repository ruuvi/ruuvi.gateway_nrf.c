#include "unity.h"

#include "main.h"

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

void setUp (void)
{
}

void tearDown (void)
{
}

void test_main_NeedToImplement (void)
{
    TEST_IGNORE_MESSAGE ("Need to Implement main");
}
