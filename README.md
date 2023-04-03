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
-   `build/`: CMake puts all build artifacts in here. This directory should not
    be tracked by version control.

## Running

TODO
