


























.build/pro5v328/.build/pro5v328/LibraryTest.o : ./.build/pro5v328/LibraryTest.cpp
	@echo [93m.build/pro5v328/LibraryTest.cpp[0m
	@mkdir -p .build/pro5v328/.build/pro5v328
	@/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/tools/avr/bin/avr-g++ -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=106 -I/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -I/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/arduino/variants/standard -fno-exceptions  -o $@ -c ./.build/pro5v328/LibraryTest.cpp
include .build/pro5v328/.build/pro5v328/LibraryTest.d

.build/pro5v328/LibraryTest.o : .build/pro5v328/LibraryTest.cpp
	@echo [93mLibraryTest.cpp[0m
	@mkdir -p .build/pro5v328
	@/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/tools/avr/bin/avr-g++ -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=106 -I/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -I/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/arduino/variants/standard -fno-exceptions -iquote .  -o $@ -c .build/pro5v328/LibraryTest.cpp
include .build/pro5v328/LibraryTest.d







.build/pro5v328/firmware.elf : .build/pro5v328/.build/pro5v328/LibraryTest.o .build/pro5v328/LibraryTest.o
	@echo [92mLinking firmware.elf[0m
	@/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/tools/avr/bin/avr-gcc -mmcu=atmega328p -Wl,-Os -Wl,--gc-sections -o $@ $^ -lm


.build/pro5v328/firmware.hex : .build/pro5v328/firmware.elf
	@echo [92mConverting to firmware.hex[0m
	@/cygdrive/c/Dropbox/Arduino/arduino-1.0.6/hardware/tools/avr/bin/avr-objcopy -O ihex -R .eeprom $^ $@

include .build/pro5v328/dependencies.d

all : .build/pro5v328/firmware.hex
	@true

