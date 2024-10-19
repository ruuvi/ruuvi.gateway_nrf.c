#ifndef MAIN_H
#define MAIN_H

/**
 * @brief String buffer for string representation of MAC address.
 */
typedef struct mac_addr_str_t
{
#if RI_LOG_ENABLED
    /** @brief Buffer for MAC address as string. */
    char buf[BLE_MAC_ADDRESS_LENGTH * 2 + (BLE_MAC_ADDRESS_LENGTH - 1) + 1];
#else
    char buf[1]; //!< Dummy buffer if logging is disabled.
#endif
} mac_addr_str_t;

#ifdef CEEDLING
#   define LOOP_FOREVER (0)
int app_main (void);
void on_wdt (void);
#else
#   define LOOP_FOREVER (1)
#endif

/**
 * @brief Convert MAC address to string.
 *
 * @param[in] p_mac Pointer to array of 6 bytes with MAC address.
 *
 * @return MAC address as string.
 */
mac_addr_str_t mac_addr_to_str (const uint8_t * const p_mac);

#endif // MAIN_H
