PICO_SDK_PATH=$(abspath lib/pico-sdk)
TARGET_NAME=mytest

.PHONY: compile
compile: | build
	PICO_SDK_PATH=$(PICO_SDK_PATH) cmake --build build --config Debug

build: CMakeLists.txt Makefile
	PICO_SDK_PATH=$(PICO_SDK_PATH) cmake -S . -B $@ -DCMAKE_BUILD_TYPE=Debug

.PHONY: clean
clean:
	rm -rf build

.PHONY: debug
debug: compile
	arm-none-eabi-gdb

.PHONY: flash
flash: compile
	picotool load build/$(TARGET_NAME).uf2 && picotool reboot

.PHONY: bootloader
bootloader:
	picotool reboot -u -f

.PHONY:
submodules:
	git submodule update --init
	cd $(PICO_SDK_PATH) && git submodule update --init
