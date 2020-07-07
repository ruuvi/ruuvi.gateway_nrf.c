#include "unity.h"

#include "ruuvi_interface_communication_ble_advertising.h"

void AssertEqual_rt_adv_init_t (rt_adv_init_t expected, rt_adv_init_t actual,
                                unsigned short line)
{
    UNITY_TEST_ASSERT_EQUAL_UINT8 (expected.channels, actual.channels, line,
                                   "rt_adv_init_t .channels mismatch");
    UNITY_TEST_ASSERT_EQUAL_UINT16 (expected.adv_interval_ms, actual.adv_interval_ms, line,
                                    "rt_adv_init_t adv interval mismatch");
    UNITY_TEST_ASSERT_EQUAL_INT8 (expected.adv_pwr_dbm, actual.adv_pwr_dbm, line,
                                  "rt_adv_init_t adv pwr mismatch");
    UNITY_TEST_ASSERT_EQUAL_UINT16 (expected.manufacturer_id, actual.manufacturer_id, line,
                                    "rt_adv_init_t manufacturer id mismatch");
}