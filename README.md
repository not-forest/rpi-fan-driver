# Raspberry Pi Fan Driver
## VERSION: "0.8.0.beta"

This kernel module is in the early stages of development and is designed to optimize and control the fan speed of Raspberry Pi machines through user-configurable settings. It allows for dynamic GPIO pin assignment based on user input via a character device interface, as well as selection of the PWM mode.

## Features

- [x] Basic GPIO pin initialization configuration.
- [x] Configure fan operation through user-space.
- [x] Support for both legacy and new methods to obtain a PWM device.
- [x] Fan device-tree overlay.
- [x] Control fan speed using PWM.
- [x] Multiple static PWM modes.
- [x] IOCTL support.
- [ ] Automatic overlay application.
- [x] RESERVED. Adaptive PWM mode (Handled by software).

## Tested on

- Raspberry Pi 4
- Custom Buildroot Linux kernel version 6.1.61-v8

## Usage

### Building the Module

Ensure you have the necessary cross-compilation tools and kernel headers installed for your Raspberry Pi's architecture. Adjust the `ARCH`, `COMPILER`, and `KVER` variables in the `Makefile` to match your build environment.

Clone the repository and use the following commands in the root directory of the source:

**Export the required environment variables:**

```bash
# Please refer to the Makefile for more details.
export ARCH=<your-arch>                 # Such as 'arm' for 32-bit devices and 'arm64' for 64-bit devices.
export COMPILER=<your-compiler-prefix>  # Such as 'aarch64-linux-gnu-' or 'arm-linux-gnueabi-'
export KVER=<your-kernel-version>       # Obtainable from 'uname -r'.
make

```

If your kernel headers are not located on a regular path: `/lib/modules/KVER/build/` you may choose your own.

```bash
export KDIR=<path_to_your_custom_kernel_headers>
```

Additional device-tree overlay is required for proper driver initialization. `DTO_OVERLAY` variable must be provided as a name of the .dtso file located in `dt_overlays/` directory. Some default presets for different boards might appear sooner.

```bash
export DTO_OVERLAY=<bcmXXXX-my_custom_overlay>
make dto
```

The used overlay must be compatible with the example ones or be compiled separatly. Example overlays can be configured without changing the file by exporting additional variables to select the required GPIO pins and provide their proper function.

```bash
# Please see the whole information about which values can be used together in the example dtso files or in the end of this README.
export PWM_PINS=<"18 19">
export PWN_FUNCS=<"2 2">
make dto
```

Device label for raspberry pi device must match with the one expected by driver. For this `DEV_LABEL` must be used, which is provided to both device-tree and C code for synchronization.

```bash
export DEV_LABEL=<my_fan_name>
```

Until the complete version is not implemented, the overlay must be applied manually.

## Static dt overlay.

The device-tree overlay examples are not dependent on raspberry-like OS and can be applied to any custom built kernels. If your custom bootloader supports overlays you may visit their documentation on how to apply this overlay statically. On pure linuxes there would not be another option, other than recompiling the kernel with included overlay.

## Dynamic dt overlay.

The example device trees are not dependent on any additional commands, which are not pure linux ones. If the linux was built with `CONFIG_OF_CONFIGFS` flag, one may use the following commands to test the current work of the driver:

```bash
mount -t configfs zero /sys/kernel/config/
mkdir /sys/kernel/config/device-tree/overlays/<anything>/
cat bcm2835-my_custom_overlay.dtbo > /sys/.../overlays/<anything>/dtbo
# Ignore the dmesg output on this.

```
### Setting up

Upon loading the Raspberry Pi Fan Driver using `insmod` and making sure the device-tree overlay is properly applied, it automatically selects the first available GPIO pin for fan control. However, you can change the GPIO pin by writing the desired pin number to the device file `/dev/rpifan`. For example:

```bash
echo "18" > /dev/rpifan  # (000|10010) WARN!! PWM_MODE_0 is reserved.
echo "237" > /dev/rpifan  # (111|01101) Set GPIO pin to 13 with PWM mode 7 (NO PWM).
echo "44" > /dev/rpifan  # (001|01100) Set GPIO pin to 12 with PWM mode 1 (Somewhere around 10%).
```

Driver will request a `pwm-bcm2835` kernel module, which will allow it for obtaining one or two PWM channels. Based on the compiled device-tree overlay the GPIO pins on which PWM will appear may vary. The adaptive PWM and custom PWM values can be adjusted via `rpi_fan_util` utility.

The value must be a `u8` written in decimal format. The first 5 bits of which, represent the address of the available pin, while the remaining 3 bits indicate the PWM mode. 

- The PWM mode `0b000` corresponds to an adaptive PWM mode, which dynamically adjusts the PWM value based on the CPU load. This mode is reserved for future use from a user level application.
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
|              |            | `0b000`: RESERVED! Adaptive PWM mode (Dynamically adjusts based on CPU load)                            |
|              |            | `0b111`: PWM_OFF (no PWM just static 3.3V)                                                    |
|              |            | Values `0b001` to `0b110`: Static PWM values calculated in percentages.                       |


### device-tree overlay additional.

Device overlays must be properly written, otherwise the driver can acknowledge that the PWM is obtained and working properly in the dmesg, hovewer nothing would've been seen on the GPIO pin. Here is the information from the `bcm2711-rpifan.dtso` file, which is compatible with raspberry pi 4:

    Additional overlay for raspberry pi pwm devices. This dt overlay is compatible with bcm2711 (Raspberry Pi 4). 

    This overlay initializes the fan device as well as acts as a "pwm-2chan" alternative overlay for blank kernels. This 
    overlay may act as a 1-channel and 2-channel overlay based on the parameters. The overlay is assumed for bcm2835's 
    PWM0, which is used primarly for GPIO pins. This config can be tweaked for PWM1, hovewer it only handles GPIO40 and 
    GPIO41, which are not part of the main GPIO header.

    Legal pins and their function combinations for each channel (May differ on other broadcom boards):
    
        ## PWM0 channel 0/1 Combinations

        | **PWM0_0 GPIO Pin** | **PWM0_0 Function (Alt)** | **PWM0_1 GPIO Pin** | **PWM0_1 Function (Alt)** |
        |---------------------|---------------------------|---------------------|---------------------------|
        | 18                  | 2 (Alt5)                  | 19                  | 2 (Alt5)                  |
        | 12                  | 4 (Alt0)                  | 13                  | 4 (Alt0)                  |
        | --                  | --                        | 45                  | 4 (Alt0)                  |
        | 52                  | 5 (Alt1)                  | 53                  | 5 (Alt1)                  |
        
        ## PWM1 channel 0/1 Combinations.
        
        | **PWM1_0 GPIO Pin** | **PWM1_0 Function (Alt)** | **PWM1_1 GPIO Pin** | **PWM1_1 Function (Alt)** |
        |---------------------|---------------------------|---------------------|---------------------------|
        | 40                  | 4 (Alt0)                  | 41                  | 4 (Alt0)                  |
    
    All defines are located in the file bottom.

