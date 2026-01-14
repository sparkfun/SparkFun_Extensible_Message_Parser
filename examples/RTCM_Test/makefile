######################################################################
# makefile
#
# Builds the example sketch
######################################################################

.ONESHELL:
SHELL=/bin/bash

##########
# Source files
##########

SKETCH=RTCM_Test
ESP32_CHIP=esp32
PARTITION_FILE_NAME=RTKEverywhere

##########
# OS specific paths
##########

ifeq ($(OS),Windows_NT)
#---------
# Windows NT generic paths
#---------

USER_DIRECTORY_PATH=C:\Users\$(USERNAME)\
EXAMPLE_PATH=..\

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Documents\Arduino\
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries\
BUILD_PATH=build\
BIN_PATH=$BUILD_PATH)esp32.esp32.$(ESP32_CHIP)\
BOOT_LOADER_PATH=..\..\binaries\
ESPTOOL_PATH=$(USER_DIRECTORY_PATH)\.arduino15\packages\esp32\tools\esptool_py\4.5.1\
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH)AppData\Local\Arduino15\packages\esp32\
READ_MAP_FILE_PATH=

# Windows NT utilities
ARDUINO_CLI=~/Arduino/arduino-cli
CLEAR=cls
COPY=copy
DELETE=rmdir /s
DIR_LISTING=dir
TERMINAL_APP=

TERMINAL_PORT=COM3
TERMINAL_PARAMS=

else
#---------
# Linux generic paths
#---------

USER_DIRECTORY_PATH=~/
EXAMPLE_PATH=../

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Arduino/
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries/
BUILD_PATH=build/
BIN_PATH=$(BUILD_PATH)esp32.esp32.$(ESP32_CHIP)/
BOOT_LOADER_PATH=~/SparkFun/SparkFun_RTK_Firmware_Uploader/RTK_Firmware_Uploader/resource/
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools/esp32-arduino-libs/
ESPTOOL_PATH=$(USER_DIRECTORY_PATH)Arduino/hardware/espressif/esp32/tools/esptool/
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/
READ_MAP_FILE_PATH=$(USER_DIRECTORY_PATH)SparkFun/rc/RTK/Firmware/Tools/

# Linux utilities
ARDUINO_CLI=$(USER_DIRECTORY_PATH)bin/arduino-cli
CLEAR=clear
COPY=cp
DELETE=rm -Rf
DIR_LISTING=ls
TERMINAL_APP=minicom

TERMINAL_PORT=/dev/ttyUSB0
TERMINAL_PARAMS=-b 115200 -8 < /dev/tty

endif

#---------
# OS Independent
#---------

# Files
BIN_FILE=$(BIN_PATH)$(SKETCH).ino.bin

##########
# Buid all the sources - must be first
##########

EXECUTABLES += $(BIN_FILE)

.PHONY: all

all: $(EXECUTABLES)

########
# Build the Firmware
##########

#DEBUG_LEVEL=debug
DEBUG_LEVEL=none

$(BIN_FILE):	$(SKETCH).ino   *.ino   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	$(ARDUINO_CLI)                                                \
		compile                                                   \
		--fqbn   "esp32:esp32:$(ESP32_CHIP)":DebugLevel=$(DEBUG_LEVEL)  \
		$<                                                        \
		--warnings default                                        \
		--build-property menu.PSRAM.enabled=Enabled               \
		--build-property build.partitions=$(PARTITION_FILE_NAME)  \
		--build-property build.flash_freq=80m                     \
		--build-property build.flash_mode=dio                     \
		--build-property build.flash_size=4MB                     \
		--build-property upload.speed=921600                      \
		--export-binaries

##########
# Upload the firmware
##########

.PHONY: upload

upload:	$(BIN_FILE)
	python3 $(ESPTOOL_PATH)esptool.py \
        --chip   esp32 \
        --port   $(TERMINAL_PORT) \
        --baud   921600 \
        --before   default_reset \
        --after   hard_reset \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        --flash_size detect \
        --compress \
         0x1000   $(BOOT_LOADER_PATH)RTK_Surveyor.ino.bootloader.bin \
         0x8000   $(BOOT_LOADER_PATH)RTK_Surveyor_Partitions_16MB.bin \
         0xe000   $(BOOT_LOADER_PATH)boot_app0.bin \
        0x10000   $<
	$(TERMINAL_APP)   -D $(TERMINAL_PORT)   $(TERMINAL_PARAMS)

##########
# Open the terminal (tty)
##########

.PHONY: terminal

terminal:
	$(TERMINAL_APP)   -D $(TERMINAL_PORT)   $(TERMINAL_PARAMS)

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -Rf *.o *.a $(BUILD_PATH)
