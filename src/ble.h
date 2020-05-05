#ifndef BLE_H_
#define BLE_H_

#include <stdint.h>

void gatt_init (void);
void ble_stack_init (void);
void scan_init (void);
void scan_start (void);
void set_company_id (uint16_t);
void clear_adv_filter();

#endif