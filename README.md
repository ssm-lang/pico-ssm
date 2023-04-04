# pico-ssm

A basic CMake project that integrates [pico-sdk][pico-sdk] with
[ssm-runtime][ssm-runtime].

[pico-sdk]: https://github.com/raspberrypi/pico-sdk
[ssm-runtime]: https://github.com/ssm-lang/ssm-runtime

## Dependencies

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

## File Organization

-   `src/`: contains `.c` files used to build the program. `CMakeLists.txt`
    inside explicitly lists compilation units to be compiled.
-   `lib/`: contains external dependencies, added as Git submodules.
    `CMakeLists.txt` declares `ssm-runtime` as a CMake library that is
    statically linked into the main program.
-   `pio/`: contains `.pio` files implementing PIO state machine code. These are
    assembled into C header files that can be included in C code and written to
    PIO program memory. `CMakeLists.txt` inside explicitly lists `.pio` files to
    be assembled.
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

## Debugging

You can debug programs on the Pico using a debug probe such as a
[Black Magic Debug][black-magic-debug], an [ORBTrace Mini][orbtrace-mini], or
a [picoprobe][picoprobe] (which is just another Pico flashed with the picoprobe
program).

[black-magic-debug]: https://black-magic.org/index.html
[orbtrace-mini]: https://orbcode.org/orbtrace-mini/
[picoprobe]: https://github.com/raspberrypi/picoprobe

You will need to connect your Pico to the debug probe via the Pico's SWD pins,
and then connect the debug probe to your computer via USB, like this:

```
[ pico ]  ===SWD==> [ debug probe ] ===USB==> [ computer ]
```

On your computer, you will interact with the pico (via the debug probe) using
a chip programmer tool like pyOCD or OpenOCD (I recommend using the former,
which is a little more plug-and-play). You will run your chip programmer as
a GDB server, which you can connect GDB to as a client.

To set all that up automatically, you can write a `.gdbinit` file. A `.gdbinit`
file contains GDB commands and is invoked automatically when you start GDB.
Your `.gdbinit` should start up pyOCD/OpenOCD in the background, connect to it,
and set up your Pico for debugging. Since your `.gdbinit` file will differ
depending on what probe and chip programmer you use, you will need maintain your
own; some samples are provided in the [`gdb/`](gdb) directory of this repo.

Once you have a proper `.gdbinit` file, you should be able to debug your program
just by running:

```
<gdb> <.elf-file>
```

where `<gdb>` is either `gdb-multiarch` (on Debian/Ubuntu) or
`arm-none-eabi-gdb` (on macOS).

The GDB command is provided as a Make target for convenience:

```sh
make debug
```
