TARGET=mytest
TARGET_ELF=./build/$(TARGET).elf

GDB=$(if $(shell which arm-none-eabi-gdb),arm-none-eabi-gdb,gdb-multiarch)
PYOCD=pyocd

COMMANDS += compile
compile: | build
	cmake --build build --config Debug --parallel $$(nproc)

COMMANDS += build
build: CMakeLists.txt Makefile
	cmake -S . -B $@ -DCMAKE_BUILD_TYPE=Debug

COMMANDS += clean
clean:
	rm -rf build

### Using pyOCD + gdb

COMMANDS += flash
flash: compile
	pyocd flash -t rp2040 $(TARGET_ELF)

COMMANDS += reset
reset:
	pyocd reset -t rp2040

COMMANDS += debug
debug: compile
	killall pyocd || true
	pyocd gdb -t rp2040 -f10m &
	$(GDB) $(TARGET_ELF) \
		--init-eval-command="set arch arm" \
		--init-eval-command="target extended-remote localhost:3333" \
		--init-eval-command="set mem inaccessible-by-default off"
	killall pyocd || true

# Using picotool
# COMMANDS += flash
# flash: compile
# 	picotool load build/$(TARGET).uf2 && picotool reboot
#
# COMMANDS += bootloader
# bootloader:
# 	picotool reboot -u -f

COMMANDS += help
help:
	@echo Available make commands: $(COMMANDS)

.PHONY: $(COMMANDS)
