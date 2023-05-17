# Serial-Blink1

Uses a rudimentary Pico-specific `ssm_platform_entry()` function that
runs a simple sslang program that calls `printf()` and `gpio_put()` to
print things to the serial port and toggles a GPIO pin.

Verifies the operation of the 64-bit hardware timer and sleeping.
