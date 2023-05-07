# Serial Port "blink" example for the rp2040 (Raspberry Pi Pico)

Uses Pico SDK code for calling `printf()`, toggling a GPIO output pin,
and waiting (use `sleep_ms()`)

# Compiling

Compile with "make" in the directory above

Binaries are placed in build/serial-blink

# Flashing .uf2 file

The "Bootsel" button and power on the Pico.  Appears as a USB drive

$ cp build/serial-blink/serial-blink.uf2 /media/sedwards/RPI-RP2/

Can also hold "boot" and press "reset"

# USB Serial port

Appears as /dev/ttyACM0

115200 baud by default

Requires, in CMakeLists.txt
```
pico_enable_stdio_usb(serial-blink 1)
pico_enable_stdio_uart(serial-blink 0)

pico_add_extra_outputs(serial-blink)
```

