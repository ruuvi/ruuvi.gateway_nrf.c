#ifndef UART_H
#define UART_H

#define UART_BUFFER_MAX_LEN 256
#define UART_OUT_MAX 200

#define STX 0x02
#define ETX 0x03
#define CMD1 1
#define CMD2 2
#define CMD1_LEN 3
#define CMD2_LEN 1

void uart_init (void);
void uart_send_broadcast (const uint8_t * mac, uint8_t * data, int len, int rssi);

#endif