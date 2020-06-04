#include "unity.h"

#include "app_uart.h"

#include "ruuvi_endpoint_ca_uart.h"
#include "ruuvi_interface_communication_ble_advertising.h"

/** https://stackoverflow.com/questions/3385515/static-assert-in-c, Nordic Mainframe */
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]

/** https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c Joey Adams*/
#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

STATIC_ASSERT(MEMBER_SIZE(ri_adv_scan_t, addr) == MEMBER_SIZE(re_ca_uart_ble_adv_t, mac),\
              Size_mismatch);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_app_uart_NeedToImplement(void)
{
    TEST_IGNORE_MESSAGE("Need to Implement app_uart");
}
