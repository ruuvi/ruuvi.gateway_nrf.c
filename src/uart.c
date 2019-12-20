#include "app_uart.h"
#include "app_scheduler.h"
#include "nrf_error.h"
#include "nrf_queue.h"
#include "uart.h"
#include "ble.h"
#include "nrf_drv_uart.h"
#include <app_fifo.h>

#define NRF_LOG_MODULE_NAME uart
#define NRF_LOG_LEVEL 4
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();


#define RX_PIN_NUMBER  11
#define TX_PIN_NUMBER  12
#define CTS_PIN_NUMBER NRF_UART_PSEL_DISCONNECTED
#define RTS_PIN_NUMBER NRF_UART_PSEL_DISCONNECTED

#define UART_TX_BUF_SIZE        512                                     /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE        256                                     /**< UART RX buffer size. */

uint8_t cmds[2][2] = {
    {CMD1, CMD1_LEN},
    {CMD2, CMD2_LEN}
};
#define NUM_OF_CMDS 2

app_fifo_t esp_cmd_fifo;
uint8_t esp_cmd_fifo_buf[32];

void process_uart_out(void * p_event_data, uint16_t event_size)
{
    uint32_t err_code = 0;
    char* buf = p_event_data;

    int len = strlen(buf);
    buf[event_size] = '\n';
    buf[event_size+1] = 0;

    NRF_LOG_INFO("process_uart_out: len: %d, strlen: %d, advertisement: %s", event_size, len, buf);

    for (uint32_t i = 0; i < event_size+1; i++) {
        bool no_mem = false;
        do {
            err_code = app_uart_put(buf[i]);
            if (err_code == NRF_ERROR_NO_MEM) {
                //print the error only once, otherwise it could repeat too many times
                if (!no_mem) {
                    NRF_LOG_ERROR("UART TX buffer full, trying again");
                    no_mem = true;
                }
            } else if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY)) {
                NRF_LOG_ERROR("Failed sending data to UART: %d", err_code);
                APP_ERROR_CHECK(err_code);
            }

        } while (err_code == NRF_ERROR_BUSY || err_code == NRF_ERROR_NO_MEM);
    }
}

void uart_send_broadcast(const uint8_t* mac, uint8_t* data, int data_len, int rssi)
{
    //NRF_LOG_DEBUG("Sending uart, len: %d", data_len);
    //NRF_LOG_HEXDUMP_DEBUG(data, data_len);
    //uint32_t err_code;
    char buf[UART_OUT_MAX] = { 0 };
    memset(buf, 0, UART_OUT_MAX);
    int index = 0;
    const int mac_len = 6;

    //beacon mac address
    for (int i=mac_len; i>0; i--, index+=2) {
        sprintf(buf+index, "%02x", mac[i-1]);
    }
    buf[index++] = ',';

    //advertisement data
    int j = index;
    for (int i=0; i<data_len; i++, index+=2) {
        sprintf(buf+j+(i*2), "%02x", data[i]);
    }
    buf[index++] = ',';

    //rssi
    index += sprintf(buf+index, "%d", rssi);

    buf[index++] = ',';
    //buf[index++] = '\r';
    //buf[index++] = '\n';
    buf[index] = 0;

    app_sched_event_put(buf, index, process_uart_out);
}

void process_uart_in(void * p_event_data, uint16_t event_size)
{
    char* buf = p_event_data;
    int len = event_size;

    NRF_LOG_DEBUG("process_uart_in: len: %d", len);
    NRF_LOG_HEXDUMP_DEBUG(buf, len);

    if (buf[0] == STX && buf[len-1] == ETX) {
        //msg valid
        uint8_t cmd = buf[2];
        uint8_t cmd_len = buf[1];
        switch(cmd) {
            case CMD1:
                if (cmd_len == CMD1_LEN) {
                    uint16_t com_id;
                    memcpy(&com_id, &buf[3], 2);
                    set_company_id(com_id);
                } else {
                    NRF_LOG_ERROR("CMD1 invalid len");
                }
            break;
            case CMD2:
                if (cmd_len == CMD2_LEN) {
                    clear_adv_filter();
                } else {
                    NRF_LOG_ERROR("CMD2 invalid len");
                }
            break;
            default:
            break;
        }
    }
}

const int shortest_cmd_len = 1;

bool is_valid_cmd(uint8_t* buf, int len)
{
    for (int i=0; i < len-shortest_cmd_len-2; i++) {
        if (buf[i] != STX) {
            continue;
        }

        int cmd_len = buf[i+1];
        if ((i+cmd_len+2) > len-1) {
            continue;
        }
        if (buf[i+cmd_len+2] != ETX) {
            continue;
        }
        uint8_t cmd = buf[i+2];

        for (int j=0; j<NUM_OF_CMDS; j++) {
            if ((cmd == cmds[j][0]) && (cmd_len == cmds[j][1])) {
            return true;
            }
        }
    }
    return false;
}

static void uart_event_handle(app_uart_evt_t * p_event)
{
    uint32_t err_code = 0;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY: {
            uint8_t b;
            UNUSED_VARIABLE(app_uart_get(&b));

            err_code = app_fifo_put(&esp_cmd_fifo, b);
            APP_ERROR_CHECK(err_code);
            if (err_code == NRF_ERROR_NO_MEM) {
                NRF_LOG_INFO("fifo no mem");
                uint8_t temp;
                uint32_t s = 1;
                err_code = app_fifo_read(&esp_cmd_fifo, &temp, &s);
                APP_ERROR_CHECK(err_code);

                err_code = app_fifo_put(&esp_cmd_fifo, b);
                APP_ERROR_CHECK(err_code);
            } else {
                APP_ERROR_CHECK(err_code);
            }

            uint8_t buf[32];
            uint32_t len;
            err_code = app_fifo_read(&esp_cmd_fifo, 0, &len);
            APP_ERROR_CHECK(err_code);

            for (int i=0; i<len; i++) {
                err_code = app_fifo_peek(&esp_cmd_fifo, i, &buf[i]);
                APP_ERROR_CHECK(err_code);
            }

            if (is_valid_cmd(buf, len)) {
                app_sched_event_put(buf, len, process_uart_in);

                err_code = app_fifo_flush(&esp_cmd_fifo);
                APP_ERROR_CHECK(err_code);
            }

            break;
        }

        case APP_UART_COMMUNICATION_ERROR:
            NRF_LOG_ERROR("Communication error occurred while handling UART.");
            //APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            NRF_LOG_ERROR("Error occurred in FIFO module used by UART.");
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the UART. */
void uart_init(void)
{
    ret_code_t err_code;

    err_code = app_fifo_init(&esp_cmd_fifo, esp_cmd_fifo_buf, 32);
    APP_ERROR_CHECK(err_code);

    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
        //.baud_rate    = UART_BAUDRATE_BAUDRATE_Baud1M
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);
}