#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "ruuvi_boards.h"

/**
 * @defgroup Configuration Application configuration.
 */

/** @{ */
/**
 * @defgroup app_config Application configuration
 * @brief Configure application enabled modules and parameters.
 */
/** @ }*/
/**
 * @addtogroup SDK15
 */
/** @{ */
/**
 * @file app_config.h
 * @author Otso Jousimaa <otso@ojousima.net>
 * @date 2020-05-06
 * @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 */

/** @brief If watchdog is not fed at this interval or faster, reboot */
#ifndef APP_WDT_INTERVAL_MS
#   define APP_WDT_INTERVAL_MS (2U*60U*1000U)
#endif

#ifndef APP_NFC_ENABLED
#   define APP_NFC_ENABLED RB_NFC_INTERNAL_INSTALLED
#endif 

#ifndef APP_ADV_ENABLED
#   define APP_ADV_ENABLED (1U)
#endif

#ifndef APP_GATT_ENABLED
#   define APP_GATT_ENABLED (0U)
#endif

#define RT_ADV_ENABLED APP_ADV_ENABLED

#ifndef RI_ADV_EXTENDED_ENABLED
#   define RI_ADV_EXTENDED_ENABLED (1U)
#endif

/** @brief enable nRF15 SDK implementation of drivers */
#define RUUVI_NRF5_SDK15_ENABLED (1U)

/** @brief enable Ruuvi Button tasks. Reset button works regardless of this setting. */
#ifndef RT_BUTTON_ENABLED
#   define RT_BUTTON_ENABLED (1U)
#endif

#ifndef RI_COMM_ENABLED
#define RI_COMM_ENABLED (1U)
#endif 

/** @brief enable Ruuvi GPIO tasks. */
#ifndef RT_GPIO_ENABLED
#   define RT_GPIO_ENABLED (1U)
#endif

/**
 * @brief enable Ruuvi GPIO interface.
 *
 * Required by SPI, LED, Button and GPIO tasks.
 */
#ifndef RI_GPIO_ENABLED
#   define RI_GPIO_ENABLED (1U)
#endif

/**
 * @brief Allocate RAM for the interrupt function pointers.
 *
 * GPIOs are indexed by GPIO number starting from 1, so size is GPIO_NUM + 1.
 */
#ifndef RT_GPIO_INT_TABLE_SIZE
#    define RT_GPIO_INT_TABLE_SIZE (RB_GPIO_NUMBER + 1U)
#endif

/**
 * @brief Enable Ruuvi Flash interface on boards with enough RAM & Flash
 */
#ifndef RI_FLASH_ENABLED
#   define RI_FLASH_ENABLED (RB_APP_PAGES > 0U)
#endif

/** @brief Enable Ruuvi led tasks. */
#ifndef RT_LED_ENABLED
#   define RT_LED_ENABLED (1U)
#endif

/** @brief Allocate memory for LED pins. */
#ifndef RT_MAX_LED_CFG
#   define RT_MAX_LED_CFG RB_LEDS_NUMBER
#endif

/** @brief Enable Ruuvi NFC tasks. */
#ifndef RT_NFC_ENABLED
#   define RT_NFC_ENABLED APP_NFC_ENABLED
#endif

/** @brief Enable Ruuvi Power interface. */
#ifndef RI_POWER_ENABLED
#   define RI_POWER_ENABLED (1U)
#endif

#ifndef RI_RADIO_ENABLED
#    define RI_RADIO_ENABLED (1U)
#endif 

#ifndef RI_SCHEDULER_ENABLED
#   define RI_SCHEDULER_ENABLED (1U)
#endif 

/** @brief Enable Ruuvi Timer interface. */
#ifndef RI_TIMER_ENABLED
#   define RI_TIMER_ENABLED (1U)
#   define RI_TIMER_MAX_INSTANCES (5U)
#endif

#ifndef RI_UART_ENABLED
#   define RI_UART_ENABLED (1U)
#endif

/** @brief Enable Ruuvi Yield interface. */
#ifndef RI_YIELD_ENABLED
#   define RI_YIELD_ENABLED (1U)
#endif

/** @brief Enable Ruuvi Watchdog interface. */
#ifndef RI_WATCHDOG_ENABLED
#   define RI_WATCHDOG_ENABLED (1U)
#endif

#ifndef APP_FW_NAME
#   define APP_FW_NAME "Ruuvi FW"
#endif

/** @brief Logs reserve lot of flash, enable only on debug builds */
#ifndef RI_LOG_ENABLED
#define RI_LOG_ENABLED (0U)
#define APP_LOG_LEVEL RI_LOG_LEVEL_NONE
#endif

#define RI_LIS2DH12_ENABLED 0
#define RI_BME280_ENABLED 0
#define RI_SHTCX_ENABLED 0

/*@}*/
#endif
