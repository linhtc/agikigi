deps_config := \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/app_trace/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/aws_iot/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/bt/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/esp32/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/ethernet/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/fatfs/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/freertos/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/heap/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/libsodium/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/log/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/lwip/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/mbedtls/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/openssl/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/pthread/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/spi_flash/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/spiffs/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/tcpip_adapter/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/wear_levelling/Kconfig \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/examples/agikigi/eelfarming/main/Kconfig.projbuild \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/leon/Documents/WorkSpace/Espressif/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
