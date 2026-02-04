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

SKETCH=SBF_Test
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
ESP32_BIN_PATH=$BUILD_PATH)esp32.esp32.$(ESP32_CHIP)\
BOOT_LOADER_PATH=..\..\binaries\
ESPTOOL_PATH=$(USER_DIRECTORY_PATH)\.arduino15\packages\esp32\tools\esptool_py\4.5.1\
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH)AppData\Local\Arduino15\packages\esp32\
READ_MAP_FILE_PATH=

# Windows NT utilities
ARDUINO_CLI=~/Arduino/arduino-cli
CLEAR=cls
COPY=copy
DELETE=rmdir /s
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
ESP32_BIN_PATH=$(BUILD_PATH)esp32.esp32.$(ESP32_CHIP)/
BOOT_LOADER_PATH=~/SparkFun/SparkFun_RTK_Firmware_Uploader/RTK_Firmware_Uploader/resource/
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools/esp32-arduino-libs/
ESPTOOL_PATH=$(USER_DIRECTORY_PATH)Arduino/hardware/espressif/esp32/tools/esptool/
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/
READ_MAP_FILE_PATH=$(USER_DIRECTORY_PATH)SparkFun/rc/RTK/Firmware/Tools/

# Linux utilities
ARDUINO_CLI=$(USER_DIRECTORY_PATH)bin/arduino-cli
CLEAR=clear
DELETE=rm -Rf

TERMINAL_APP=minicom
TERMINAL_PARAMS=-8 < /dev/tty

endif

#---------
# OS Independent
#---------

# Files
ESP32_BIN_FILE=$(ESP32_BIN_PATH)$(SKETCH).ino.bin

##########
# Buid all the sources - must be first
##########

EXECUTABLES += $(ESP32_BIN_FILE)
EXECUTABLES += samd21_LoRaSerial
EXECUTABLES += samd51_thing_plus
EXECUTABLES += micromod_samd51

.PHONY: all

all: $(EXECUTABLES)

##########
# Open the terminal (tty)
##########

BAUDRATE=115200

.PHONY: terminal

terminal:
	if   [   -a   "/dev/ttyUSB0"   ];   then
	    export TERMINAL_PORT=/dev/ttyUSB0
	else
	    if   [   -a   "/dev/ttyUSB1"   ];   then
	        export TERMINAL_PORT=/dev/ttyUSB1
	    else
	        if   [   -a   "/dev/ttyUSB2"   ];   then
	            export TERMINAL_PORT=/dev/ttyUSB2
	        else
	            if   [   -a   "/dev/ttyUSB3"   ];   then
	                export TERMINAL_PORT=/dev/ttyUSB3
	            else
	                if   [   -a   "/dev/ttyACM0"   ];   then
	                    export TERMINAL_PORT=/dev/ttyACM0
	                else
	                    if   [   -a   "/dev/ttyACM1"   ];   then
	                        export TERMINAL_PORT=/dev/ttyACM1
	                    else
	                        if   [   -a   "/dev/ttyACM2"   ];   then
	                            export TERMINAL_PORT=/dev/ttyACM2
	                        else
	                            if   [   -a   "/dev/ttyACM3"   ];   then
	                                export TERMINAL_PORT=/dev/ttyACM3
	                            fi
	                        fi
	                    fi
	                fi
	            fi
	        fi
	    fi
	fi
	echo $$TERMINAL_PORT
	$(TERMINAL_APP)   -D $$TERMINAL_PORT   -b $(BAUDRATE)   $(TERMINAL_PARAMS)

##########
# Build the ESP32 Firmware
##########

#DEBUG_LEVEL=debug
DEBUG_LEVEL=none

$(ESP32_BIN_FILE):	$(SKETCH).ino   *.h   *.ino   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	echo $(SKETCH) - ESP32
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
# Upload the ESP32 firmware
##########

.PHONY: upload

upload:	$(ESP32_BIN_FILE)
	if   [   -a   "/dev/ttyUSB0"   ];   then
	    export TERMINAL_PORT=/dev/ttyUSB0
	else
	    if   [   -a   "/dev/ttyUSB1"   ];   then
	        export TERMINAL_PORT=/dev/ttyUSB1
	    else
	        if   [   -a   "/dev/ttyUSB2"   ];   then
	            export TERMINAL_PORT=/dev/ttyUSB2
	        else
	            if   [   -a   "/dev/ttyUSB3"   ];   then
	                export TERMINAL_PORT=/dev/ttyUSB3
	            else
	                if   [   -a   "/dev/ttyACM0"   ];   then
	                    export TERMINAL_PORT=/dev/ttyACM0
	                else
	                    if   [   -a   "/dev/ttyACM1"   ];   then
	                        export TERMINAL_PORT=/dev/ttyACM1
	                    else
	                        if   [   -a   "/dev/ttyACM2"   ];   then
	                            export TERMINAL_PORT=/dev/ttyACM2
	                        else
	                            if   [   -a   "/dev/ttyACM3"   ];   then
	                                export TERMINAL_PORT=/dev/ttyACM3
	                            fi
	                        fi
	                    fi
	                fi
	            fi
	        fi
	    fi
	fi
	echo $$TERMINAL_PORT
	python3 $(ESPTOOL_PATH)esptool.py \
        --chip   esp32 \
        --port   $$TERMINAL_PORT \
        --baud   921600 \
        --before   default_reset \
        --after   hard_reset \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        --flash_size detect \
        --compress \
         0x1000   $(BOOT_LOADER_PATH)RTK_Surveyor.ino.bootloader.bin \
         0x8000   $(BOOT_LOADER_PATH)RTK_Surveyor_Partitions_4MB.bin \
         0xe000   $(BOOT_LOADER_PATH)boot_app0.bin \
        0x10000   $<
	$(TERMINAL_APP)   -D $$TERMINAL_PORT   -b $(BAUDRATE)   $(TERMINAL_PARAMS)

##########
# Build the SAMD21 Firmware
##########

samd21_LoRaSerial:	$(SKETCH).ino   *.h   *.ino   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	echo $(SKETCH) - $@
	$(ARDUINO_CLI)                          \
		compile                             \
		$<                                  \
		--fqbn   SparkFun:samd:LoRaSerial

##########
# Upload the SAMD21 firmware
##########

samd21_LoRaSerial_upload:	$(SKETCH).ino   samd21_LoRaSerial
	if   [   -a   "/dev/ttyUSB0"   ];   then
	    export TERMINAL_PORT=/dev/ttyUSB0
	else
	    if   [   -a   "/dev/ttyUSB1"   ];   then
	        export TERMINAL_PORT=/dev/ttyUSB1
	    else
	        if   [   -a   "/dev/ttyUSB2"   ];   then
	            export TERMINAL_PORT=/dev/ttyUSB2
	        else
	            if   [   -a   "/dev/ttyUSB3"   ];   then
	                export TERMINAL_PORT=/dev/ttyUSB3
	            else
	                if   [   -a   "/dev/ttyACM0"   ];   then
	                    export TERMINAL_PORT=/dev/ttyACM0
	                else
	                    if   [   -a   "/dev/ttyACM1"   ];   then
	                        export TERMINAL_PORT=/dev/ttyACM1
	                    else
	                        if   [   -a   "/dev/ttyACM2"   ];   then
	                            export TERMINAL_PORT=/dev/ttyACM2
	                        else
	                            if   [   -a   "/dev/ttyACM3"   ];   then
	                                export TERMINAL_PORT=/dev/ttyACM3
	                            fi
	                        fi
	                    fi
	                fi
	            fi
	        fi
	    fi
	fi
	echo $$TERMINAL_PORT
	$(ARDUINO_CLI)                          \
        upload   $<                         \
        -p   $$TERMINAL_PORT                \
        -b   SparkFun:samd:LoRaSerial
	$(TERMINAL_APP)   -D $$TERMINAL_PORT   -b $(BAUDRATE)   $(TERMINAL_PARAMS)

##########
# Build the SAMD51 Firmware
##########

samd51_thing_plus:	$(SKETCH).ino   *.h   *.ino   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	echo $(SKETCH) - $@
	$(ARDUINO_CLI)                                  \
		compile                                     \
		$<                                          \
		--fqbn   SparkFun:samd:samd51_thing_plus

##########
# Upload the SAMD51 firmware
##########

samd51_thing_plus_upload:	$(SKETCH).ino   samd51_thing_plus
	if   [   -a   "/dev/ttyUSB0"   ];   then
	    export TERMINAL_PORT=/dev/ttyUSB0
	else
	    if   [   -a   "/dev/ttyUSB1"   ];   then
	        export TERMINAL_PORT=/dev/ttyUSB1
	    else
	        if   [   -a   "/dev/ttyUSB2"   ];   then
	            export TERMINAL_PORT=/dev/ttyUSB2
	        else
	            if   [   -a   "/dev/ttyUSB3"   ];   then
	                export TERMINAL_PORT=/dev/ttyUSB3
	            else
	                if   [   -a   "/dev/ttyACM0"   ];   then
	                    export TERMINAL_PORT=/dev/ttyACM0
	                else
	                    if   [   -a   "/dev/ttyACM1"   ];   then
	                        export TERMINAL_PORT=/dev/ttyACM1
	                    else
	                        if   [   -a   "/dev/ttyACM2"   ];   then
	                            export TERMINAL_PORT=/dev/ttyACM2
	                        else
	                            if   [   -a   "/dev/ttyACM3"   ];   then
	                                export TERMINAL_PORT=/dev/ttyACM3
	                            fi
	                        fi
	                    fi
	                fi
	            fi
	        fi
	    fi
	fi
	echo $$TERMINAL_PORT
	$(ARDUINO_CLI)                              \
        upload   $<                             \
        -p   $$TERMINAL_PORT                    \
        -b   SparkFun:samd:samd51_thing_plus
	$(TERMINAL_APP)   -D $$TERMINAL_PORT   -b $(BAUDRATE)   $(TERMINAL_PARAMS)

##########
# Build the MicroMod SAMD51 Firmware
##########

micromod_samd51:	$(SKETCH).ino   *.h   *.ino   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	echo $(SKETCH) - $@
	$(ARDUINO_CLI)                                  \
		compile                                     \
		$<                                          \
		--fqbn   SparkFun:samd:micromod_samd51

##########
# Upload the MicroMod SAMD51 firmware
##########

micromod_samd51_upload:	$(SKETCH).ino   micromod_samd51
	if   [   -a   "/dev/ttyUSB0"   ];   then
	    export TERMINAL_PORT=/dev/ttyUSB0
	else
	    if   [   -a   "/dev/ttyUSB1"   ];   then
	        export TERMINAL_PORT=/dev/ttyUSB1
	    else
	        if   [   -a   "/dev/ttyUSB2"   ];   then
	            export TERMINAL_PORT=/dev/ttyUSB2
	        else
	            if   [   -a   "/dev/ttyUSB3"   ];   then
	                export TERMINAL_PORT=/dev/ttyUSB3
	            else
	                if   [   -a   "/dev/ttyACM0"   ];   then
	                    export TERMINAL_PORT=/dev/ttyACM0
	                else
	                    if   [   -a   "/dev/ttyACM1"   ];   then
	                        export TERMINAL_PORT=/dev/ttyACM1
	                    else
	                        if   [   -a   "/dev/ttyACM2"   ];   then
	                            export TERMINAL_PORT=/dev/ttyACM2
	                        else
	                            if   [   -a   "/dev/ttyACM3"   ];   then
	                                export TERMINAL_PORT=/dev/ttyACM3
	                            fi
	                        fi
	                    fi
	                fi
	            fi
	        fi
	    fi
	fi
	echo $$TERMINAL_PORT
	$(ARDUINO_CLI)                              \
        upload   $<                             \
        -p   $$TERMINAL_PORT                    \
        -b   SparkFun:samd:micromod_samd51
	$(TERMINAL_APP)   -D $$TERMINAL_PORT   -b $(BAUDRATE)   $(TERMINAL_PARAMS)

##########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -Rf *.o *.a $(BUILD_PATH)
