# pico-ssm

RP2040 bindings for [sslang][sslang] and its [ssm-runtime][ssm-runtime].

[ssm-runtime]: https://github.com/ssm-lang/ssm-runtime
[sslang]: https://github.com/ssm-lang/sslang

## Dependencies

You will need the Sslang compiler, `sslc`, to generate SSM programs that link
against the `ssm-runtime`. You should build and install it [from
source][sslang], and make it available on your `PATH`. (Alternatively, you may
[patch the CMakeLists.txt](programs/CMakeLists.txt#L34) to point directly to
where `sslc` is installed.)

This project needs to be built using Raspberry Pi's [Pico SDK][pico-sdk]. We've
vendored that library in this repo so you won't need to obtain it separately,
but you will need to install its dependencies yourself:

[pico-sdk]: https://github.com/raspberrypi/pico-sdk

### macOS

Install CMake and the Arm Embedded Toolchain:

```
brew install cmake
brew install --cask gcc-arm-embedded
```

### Ubuntu/Debian

```sh
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

If you want to use GDB for debugging, also install the multi-architecture version:

```sh
sudo apt install gdb-multiarch
```

## Building

Just Run Make™:

```sh
make
```

The top-level `Makefile` handles setting up the build directory and invoking the
build command, all using CMake. CMake automatically handles initializing this
repo's Git submodules (containing pico-sdk and ssm-runtime), and also sets the
`PICO_SDK_PATH` variable that tells CMake where pico-sdk can be found.

## Directory Organization

-   `ssm-rp2040/`: contains the SSM runtime bindings for the RP2040 platform,
    including `.pio` files implementing PIO state machine code.

-   `programs/`: contains a collection of Sslang programs, which it
    automatically builds using `sslc`. You may write your own Sslang programs
    and add them here. Note that some of the Sslang programs have hard-coded
    GPIO pin numbers, which you may want to adjust according to the RP2040
    hardware you are using.

-   `lib/`: contains external dependencies, added as Git submodules.
    `CMakeLists.txt` declares `ssm-runtime` as a CMake library that is
    statically linked into the main program.

-   `tests/`: a collection of test suites created during the development
    process. Overdue for cleanup.

-   `cmake/:` contains additional `.cmake` files used by the top-level
    `CMakeLists.txt` file.

-   `gdb/`: contains sample `.gdbinit` files.

-   `build/`: CMake puts all build artifacts in here. This directory should not
    be tracked by version control.

## Running

If you have your Pico connected to your machine via the USB port, you can boot
it in bootloader mode (power it on while holding the BOOTSEL button). It should
appear as a mountable disk drive, in which you can place the compiled `.uf2`
file to flash the microcontroller. You can also automate this process using the
[`picotool`][picotool] provided by Raspberry Pi (I recommend compiling and
installing it from source so that it is on your PATH).

[picotool]: https://github.com/raspberrypi/picotool

Alternatively, if you have your Pico connected to a debug probe via SWD, you can
flash it using tools like pyOCD or OpenOCD. For instance, with pyOCD, the
command to do so is:

```sh
pyocd flash -t rp2040 <.elf-file>
```

The pyOCD command is provided as a Make target for convenience:

```sh
make flash
```

> **NOTE** The default UART baud rate is 115200, with TX on GP0 and RX on GP1.
