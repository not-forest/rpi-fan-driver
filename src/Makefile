#	Main compilation makefile.
#
#	From here one can customize the build for a custom compiled linux running on raspberry pi device or
#	an existing pre compiled kernel. To properly compile the driver, make sure to export all symbols properly
#	based on your target device and kernel.

# Architecture must be provided. For 32-bit targets: Raspberry Pi 1 (Model A, A+, B, B+), Raspberry Pi Zero / Zero W / Zero WH and
# Raspberry Pi 2 (Model B) an 'arm' architecture must be used. For 64-bit targets: Raspberry Pi 2 (Model B), Raspberry Pi 3 (Model B, B+, A+)
# Raspberry Pi 4 (Model B) and Raspberry Pi 400 an 'arm64' must be used. For 64-bit targets leave this default option unchanged. 
ARCH ?= arm64
# Compiler 'prefix' must be given for the main driver makefile script. For 64-bit targets leave this default option unchanged. For 64-bit targets
# 'arm-linux-gnueabi-' might be a possible option. If have some specific compiler to build your custom kernel, use it instead.
COMPILER ?= aarch64-linux-gnu-

# PWM overlay for this driver. Can be ignored if your kernel is aligned to the driver. 
DTO_OVERLAY ?= bcm2711-rpifan.dtso
DTO_OVERLAY_S := ../dt_overlays/$(DTO_OVERLAY)
DTO_OVERLAY_O := ${DTO_OVERLAY_S:.dtso=.dtbo}

# A label of out fan device. This one must match with the chosen one from the dts overlay.
DEV_LABEL ?= rpifan

### PWM_GPIOS and PWM_FUNCS must be defined with. Please export those symbols written in "". Those are required for
### PWM to work properly and must be either applied dynamically or added to the device tree via bootloader commands or
### recompiling the kernel manually.

# Up to two gpios must be chosen. For list of compatible pins and their function see README or the preview overlay.
PWM_PINS ?= "12 13"
# GPIO function keys. For list of compatible pins and their function see README or the preview overlay.
PWM_FUNCS ?= "4 4"

CFLAGS += -DDEV_LABEL -DPWM_GPIOS -DPWM_PINS -DPWM_FUNCS
DTOFLAGS += -DPINS=$(PWM_PINS) -DFUNCS=$(PWM_FUNCS) -DLABEL=\"$(DEV_LABEL)\"

# Makefile for driver compilation
obj-m += rpifan.o
rpifan-objs += driver.o gpio.o pwm.o

# Kernel headers version. Can be adjusted when calling.
KVER ?= 6.1.61-v8
KDIR := /lib/modules/$(KVER)/build/

all:
	make -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) modules

dto:
	gcc -xc -E -P $(DTOFLAGS) $(DTO_OVERLAY_S) | dtc -@ -I dts -O dtb -o $(DTO_OVERLAY_O)

clean:
	make -C $(KDIR) M=$(CURDIR) clean

clean_dto:
	@rm -rf $(wildcard ../dt_overlays/*.dtbo) 
