# Source files and includes common for all targets
NRF_LIB_SOURCES= \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/ble_advertising/ble_advertising.c \
  $(SDK_ROOT)/components/ble/ble_link_ctx_manager/ble_link_ctx_manager.c \
  $(SDK_ROOT)/components/ble/ble_radio_notification/ble_radio_notification.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis/ble_dis.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus/ble_nus.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_params.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_state.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
  $(SDK_ROOT)/components/ble/nrf_ble_scan/nrf_ble_scan.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatt_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatts_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/id_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_data_storage.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_database.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_id.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/pm_buffer.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_dispatcher.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_manager.c \
  $(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
  $(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
  $(SDK_ROOT)/components/libraries/atomic_flags/nrf_atflags.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(SDK_ROOT)/components/libraries/bootloader/dfu/nrf_dfu_svci.c \
  $(SDK_ROOT)/components/libraries/crc16/crc16.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_init.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng_mbedtls.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_chacha_poly_aead.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_ecc.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon/oberon_backend_hmac.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecc.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecdsa.c \
  $(SDK_ROOT)//components/libraries/crypto/nrf_crypto_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_init.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_rng.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_uart.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
  $(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
  $(SDK_ROOT)/components/libraries/fds/fds.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
  $(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/serial/nrf_serial.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/util/sdk_mapped_flags.c \
  $(SDK_ROOT)/components/nfc/ndef/generic/message/nfc_ndef_msg.c \
  $(SDK_ROOT)/components/nfc/ndef/generic/record/nfc_ndef_record.c \
  $(SDK_ROOT)/components/nfc/ndef/parser/message/nfc_ndef_msg_parser.c \
  $(SDK_ROOT)/components/nfc/ndef/parser/message/nfc_ndef_msg_parser_local.c \
  $(SDK_ROOT)/components/nfc/ndef/parser/record/nfc_ndef_record_parser.c \
  $(SDK_ROOT)/components/nfc/ndef/launchapp/nfc_launchapp_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/text/nfc_text_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/uri/nfc_uri_msg.c \
  $(SDK_ROOT)/components/nfc/ndef/uri/nfc_uri_rec.c \
  $(SDK_ROOT)/components/nfc/platform/nfc_platform.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_rng.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_spi.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_twi.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_uart.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_nfct.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rng.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rtc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_spi.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twi.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twim.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uart.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uarte.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
  $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52.c

RUUVI_LIB_SOURCES= \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/log/ruuvi_interface_log.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/communication/ruuvi_interface_communication_radio.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/adc/ruuvi_nrf5_sdk15_adc_mcu.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/atomic/ruuvi_nrf5_sdk15_atomic.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/communication/ruuvi_nrf5_sdk15_communication.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/communication/ruuvi_nrf5_sdk15_communication_radio.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/communication/ruuvi_nrf5_sdk15_communication_ble_advertising.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/communication/ruuvi_nrf5_sdk15_communication_ble_gatt.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/communication/ruuvi_nrf5_sdk15_communication_uart.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/environmental/ruuvi_nrf5_sdk15_environmental_mcu.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/flash/ruuvi_nrf5_sdk15_flash.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/gpio/ruuvi_nrf5_sdk15_gpio.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/gpio/ruuvi_nrf5_sdk15_gpio_interrupt.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/log/ruuvi_nrf5_sdk15_log.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/power/ruuvi_nrf5_sdk15_power.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/rtc/ruuvi_nrf5_sdk15_rtc_mcu.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/ruuvi_nrf5_sdk15_error.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/scheduler/ruuvi_nrf5_sdk15_scheduler.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/ruuvi.nrf_sdk15_3_overrides.c/nrfx_wdt.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/ruuvi.nrf_sdk15_3_overrides.c/ble_dfu.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/ruuvi.nrf_sdk15_3_overrides.c/ble_dfu_bonded.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/ruuvi.nrf_sdk15_3_overrides.c/ble_dfu_unbonded.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/spi/ruuvi_nrf5_sdk15_spi.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/timer/ruuvi_nrf5_sdk15_timer.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/watchdog/ruuvi_nrf5_sdk15_watchdog.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/yield/ruuvi_nrf5_sdk15_yield.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/ruuvi_driver_error.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/ruuvi_driver_sensor.c \
  $(PROJ_DIR)/ruuvi.endpoints.c/src/ruuvi_endpoint_ca_uart.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/tasks/ruuvi_task_advertisement.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/tasks/ruuvi_task_led.c \

RUUVI_PRJ_SOURCES= \
  $(PROJ_DIR)/main.c \
  $(PROJ_DIR)/app_ble.c \
  $(PROJ_DIR)/app_uart.c

COMMON_SOURCES= \
  $(RUUVI_LIB_SOURCES) \
  $(RUUVI_PRJ_SOURCES) \
  $(NRF_LIB_SOURCES) \

COMMON_INCLUDES= \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/components/ble/ble_radio_notification \
  $(SDK_ROOT)/components/ble/ble_services/ble_dfu \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis/ \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/ble_link_ctx_manager/ \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr \
  $(SDK_ROOT)/components/ble/nrf_ble_scan \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  $(SDK_ROOT)/components/libraries/atomic_flags/ \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/libraries/bootloader \
  $(SDK_ROOT)/components/libraries/bootloader/ble_dfu/ \
  $(SDK_ROOT)/components/libraries/bootloader/dfu/ \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/libraries/crc16 \
  $(SDK_ROOT)/components/libraries/crypto \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310 \
  $(SDK_ROOT)/components/libraries/crypto/backend/cifra \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_bl \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc \
  $(SDK_ROOT)/components/libraries/crypto/backend/mbedtls \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/ \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_sw \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon \
  $(SDK_ROOT)/components/libraries/crypto/backend/optiga \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/libraries/pwr_mgmt \
  $(SDK_ROOT)/components/libraries/queue/ \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/serial/ \
  $(SDK_ROOT)/components/libraries/stack_info/ \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/libraries/svc \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/nfc/ndef/launchapp/ \
  $(SDK_ROOT)/components/nfc/ndef/generic/message \
  $(SDK_ROOT)/components/nfc/ndef/generic/record \
  $(SDK_ROOT)/components/nfc/ndef/parser/message \
  $(SDK_ROOT)/components/nfc/ndef/parser/record \
  $(SDK_ROOT)/components/nfc/ndef/text/ \
  $(SDK_ROOT)/components/nfc/ndef/uri \
  $(SDK_ROOT)/components/nfc/t4t_lib \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/softdevice/s132/headers \
  $(SDK_ROOT)/components/softdevice/s132/headers/nrf52 \
  $(SDK_ROOT)/components/toolchain/gcc \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/external/mbedtls/include \
  $(SDK_ROOT)/external/nrf_tls/mbedtls/nrf_crypto/config \
  $(SDK_ROOT)/external/nrf_cc310/include \
  $(SDK_ROOT)/external/nrf_oberon/include \
  $(SDK_ROOT)/external/nrf_oberon/ \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/integration/nrfx/legacy/ \
  $(SDK_ROOT)/modules/nrfx/ \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/modules/nrfx/mdk \
  $(SDK_ROOT)/integration/nrfx \
  $(PROJ_DIR)/config \
  $(PROJ_DIR)/ruuvi.boards.c \
  $(PROJ_DIR)/ruuvi.drivers.c/src \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/adc \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/atomic \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/communication \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/environmental \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/flash \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/gpio \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/i2c \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/log \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/power \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/rtc \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/scheduler \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/spi \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/timer \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/watchdog \
  $(PROJ_DIR)/ruuvi.drivers.c/src/interfaces/yield \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/gpio/ \
  $(PROJ_DIR)/ruuvi.drivers.c/src/nrf5_sdk15_platform/timer/ \
  $(PROJ_DIR)/ruuvi.drivers.c/src/tasks \
  $(PROJ_DIR)/ruuvi.endpoints.c/src
