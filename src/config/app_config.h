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
/** @}*/
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

/** @brief If watchdog is not fed at this interval or faster, reboot. */
#ifndef APP_WDT_INTERVAL_MS
#   define APP_WDT_INTERVAL_MS (2U*60U*1000U)
#endif

/** @brief Enable/disable NFC tag functionality. */
#ifndef APP_NFC_ENABLED
#   define APP_NFC_ENABLED RB_NFC_INTERNAL_INSTALLED
#endif

/** @brief Enable/disable BLE advertising functionality. */
#ifndef APP_ADV_ENABLED
#   define APP_ADV_ENABLED (1U)
#endif

/** @brief Enable/disable BLE GATT functionality. */
#ifndef APP_GATT_ENABLED
#   define APP_GATT_ENABLED (0U)
#endif

/** @brief Route USB to PC via interface MCU on PCA boards. */
#ifndef APP_USBUART_ENABLED
#   if defined (BOARD_PCA10040)
#       define APP_USBUART_ENABLED (1U)
#   elif defined (BOARD_PCA10056E)
#       define APP_USBUART_ENABLED (1U)
#   else
#       define APP_USBUART_ENABLED (0U)
#   endif
#endif

/** @brief Enable Ruuvi advertising tasks. */
#define RT_ADV_ENABLED APP_ADV_ENABLED

/**
 * @brief Enable extended advertising, i.e. Long Range and 2 MBit / s.
 *
 * Reserves a lot of memory for data payloads in program, 255 bytes vs 31 bytes.
 */
#ifndef RI_ADV_EXTENDED_ENABLED
#   define RI_ADV_EXTENDED_ENABLED (1U)
#endif

/** @brief Enable Ruuvi GATT tasks. */
#ifndef RT_GATT_ENABLED
#   define RT_GATT_ENABLED APP_GATT_ENABLED
#endif

/** @brief enable nRF15 SDK implementation of drivers */
#define RUUVI_NRF5_SDK15_ENABLED (1U)

/** @brief enable Ruuvi Button tasks. Reset button works regardless of this setting. */
#ifndef RT_BUTTON_ENABLED
#   define RT_BUTTON_ENABLED (1U)
#endif

/** @brief Enable Ruuvi communication tasks. */
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
#   define RI_FLASH_ENABLED (0U)
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

/** @brief Enable Radio interface. */
#ifndef RI_RADIO_ENABLED
#    define RI_RADIO_ENABLED (1U)
#endif

/** @brief Enable Scheduler interface. */
#ifndef RI_SCHEDULER_ENABLED
#   define RI_SCHEDULER_ENABLED (1U)
#endif

/** @brief Maximum number of tasks in scheduler. */
#ifndef RI_SCHEDULER_LENGTH
#   define RI_SCHEDULER_LENGTH (10U)
#endif

/**
 * @brief Maximum data length of scheduled task.
 *
 * Must accomodate extended advertisement.
 */
#ifndef RI_SCHEDULER_SIZE
#  define RI_SCHEDULER_SIZE (255U)
#endif


/**
 * @brief Enable Ruuvi Timer interface.
 */
#ifndef RI_TIMER_ENABLED
#   define RI_TIMER_ENABLED (1U)
/** @brief Each instance reserves RAM, runs on same physical timer. */
#   define RI_TIMER_MAX_INSTANCES (5U)
#endif

/** @brief Enable Ruuvi UART interface */
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

/** @brief Name for firmware. */
#ifndef APP_FW_NAME
#   define APP_FW_NAME "Ruuvi GW"
#endif

/** @brief Logs reserve lot of flash, enable only on debug builds. */
#ifndef RI_LOG_ENABLED
#   ifdef DEBUG
#       define RI_LOG_ENABLED ((RB_APP_PAGES > 0U))
/** @brief Log verbosity level */
#       define APP_LOG_LEVEL RI_LOG_LEVEL_INFO
#   else
#       define RI_LOG_ENABLED (0)
/** @brief Log verbosity level */
#       define APP_LOG_LEVEL RI_LOG_LEVEL_NONE
#   endif
#endif

/** @brief Disable LIS2DH12 code explicitly */
#define RI_LIS2DH12_ENABLED 0
/** @brief Disable BME280 code explicitly */
#define RI_BME280_ENABLED 0
/** @brief Disable SHTCX code explicitly */
#define RI_SHTCX_ENABLED 0
/** @brief Disable DPS310 code explicitly */
#define RI_DPS310_ENABLED 0

/** @} */
#endif
