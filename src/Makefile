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
# WARN: Kernel <4.5.0 users only. Uses old legacy pwm api. Ignored on newer kernels.
PWM_INDEX ?= 0

CFLAGS += -DDEV_LABEL
CFLAGS += -DPWM_INDEX

# Makefile for driver compilation
obj-m += rpifan.o
rpifan-objs += driver.o gpio.o pwm.o

# Kernel headers version. Can be adjusted when calling.
KVER ?= 6.1.61-v8
KDIR := /lib/modules/$(KVER)/build/

all:
	make -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) modules

dto: 
	dtc -@ -I dts -O dtb -o $(DTO_OVERLAY_O) $(DTO_OVERLAY_S)

clean:
	make -C $(KDIR) M=$(CURDIR) clean

clean_dto:
	@rm -rf $(wildcard ../dt_overlays/*.dtbo) 
