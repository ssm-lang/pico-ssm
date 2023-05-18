# Sslang programs

To add a new program, just add a new `.ssl` source file here. The
`CMakeLists.txt` here will pick it up (via a file name glob) and compile an
executable with it.

A sslang program named `foo.ssl` will produce an executable named `foo.elf` in
`build/programs/`.

## Initialization

### Using GPIO pins

To use GPIO pins, make sure to declare the following function:

```
extern rp2040_io_init : (Int, Int) -> (Int, Int) -> (&Int, &Int)
```

Then you can call it like this:

```
let (gin, gout) = rp2040_io_init (15, 1) (25, 1)
```

The first parameter is `(input_base, input_count)`, and the second is `(output_base, output_count)`.
The return value is a tuple of `Int` references, mapped to external GPIO input and output pins.

If `_count` is zero for either input or output, the corresponding return value
is still a `&Int`, but it won't be mapped to anything.

### Time units

Ticks advance at 16 MHz, so use the following to convert to milliseconds and microseconds:

```
ms t = t * 16000
us t = t * 16
```

### Sleeping

Make sure to sleep for a little bit in the first instant. FIXME we should not have to do this.

### Device-specific considerations

Make sure to adjust `pico_enable_stdio_usb()` and `pico_enable_stdio_uart()`
according to your hardware setup.

Make sure to configure input pins as pull-up if using a switch; see
[blink.ssl](blink.ssl) for an example of how this is done in sslang.

The following pin mappings are what we've used for our personal setups, but do
not have to be followed strictly; adapt accordingly.

#### Pico Pin Mappings

(John)
```c
#define INPUT_PIN_BASE 15
#define INPUT_PIN_COUNT 1
#define OUTPUT_PIN_BASE PICO_DEFAULT_LED_PIN
#define OUTPUT_PIN_COUNT 1
```

Note that `PICO_DEFAULT_LED_PIN` is 25.

#### RP2040ZERO Pin Mappings

(Stephen)
```c
#define INPUT_PIN_BASE 4
#define INPUT_PIN_COUNT 2
#define OUTPUT_PIN_BASE 14
#define OUTPUT_PIN_COUNT 1
```

#### LilyGo Pin Mappings

(Stephen)
```c
#define INPUT_PIN_BASE 6
#define INPUT_PIN_COUNT 2
#define OUTPUT_PIN_BASE 25
#define OUTPUT_PIN_COUNT 1
```
