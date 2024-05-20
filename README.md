# Raspberry Pi Fan Driver

This kernel module is in the early stages of development and is designed to optimize and control the fan speed of a Raspberry Pi machines through user-configurable settings. It allows for dynamic GPIO pin assignment based on user input via a character device interface as well as selecting the PWM mode.

## Features

- [x] Basic GPIO init pin configuration.
- [x] Configure fan operation through user-space.
- [x] Both legacy and new way to obtain PWM device is supported.
- [x] Fan device-tree overlay.
- [ ] Control fan speed using PWM.
- [ ] Multiple PWM modes.

## Tested on

- Raspberry Pi 4
- Custom Buildroot Linux kernel version 6.1.61-v8

## Usage

### Building the Module

Ensure you have the necessary cross-compilation tools and kernel headers installed for your Raspberry Pi's architecture. Adjust the `ARCH`, `COMPILER`, and `KVER` variables in the `Makefile` to match your build environment.

Clone the repository and use the following commands in the root directory of the source:

**Export the required environment variables:**

```bash
export ARCH=<your-arch>
export COMPILER=<your-compiler-prefix>
export KVER=<your-kernel-version>
make
```

For kernel versions smaller than 4.5.0, `PWM_INDEX` can be provided to chose the required PWM channel. By default this value is set to zero.

```bash
export PWM_INDEX=<index>
```

For newer kernels an additional device tree overlay is required. `DTO_OVERLAY` variable must be provided as a name of the .dtso file located in `dt_overlays/` directory. Some default presets for different boards might appear sooner.

```bash
export DTO_OVERLAY=<bcmXXXX-my_custom_overlay>
make dto
```

### Setting up

Upon loading the Raspberry Pi Fan Driver using `insmod`, it automatically selects the first available GPIO pin for fan control. However, you can change the GPIO pin by writing the desired pin number to the device file `/dev/rpifan`. For example:

```bash
echo "18" > /dev/rpifan  # Set GPIO pin to 18 with adaptive PWM.
```

The value must be a `u8` written in decimal format. The first 5 bits of which, represent the address of the available pin, while the remaining 3 bits indicate the PWM mode. 

- The PWM mode `0b000` corresponds to an adaptive PWM mode, which dynamically adjusts the PWM value based on the CPU load.
- Conversely, `0b111` signifies PWM_OFF.
- Values in between represent static PWM signal duty cycle values calculated in percentages.

Here is a more detailed structure of the expected value:

| Bit Position | Field      | Description                                                                                   |
|--------------|------------|-----------------------------------------------------------------------------------------------|
| 4 - 0        | GPIO Pin   | Represents the address of the available GPIO pin for fan control.                             |
|              |            | Valid Range: 2 to 27 (inclusive)                                                              |
|              |            | Reserved Values: 0, 1 (ID_EEPROM pins)                                                        |
|              |            | Rest are considered out-of-range                                                              |
|--------------|------------|-----------------------------------------------------------------------------------------------|
| 7 - 5        | PWM Mode   | Represents the PWM mode for fan control.                                                      |
|              |            | `0b000`: Adaptive PWM mode (Dynamically adjusts based on CPU load)                            |
|              |            | `0b111`: PWM_OFF (no PWM just static 3.3V)                                                    |
|              |            | Values `0b001` to `0b110`: Static PWM values calculated in percentages.                       |

