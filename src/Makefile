# Makefile for driver compilation
obj-m += rpifan.o
rpifan-objs += driver.o gpio.o

ARCH ?= arm64
COMPILER ?= aarch64-linux-gnu-

# Kernel headers version. Can be adjusted when calling.
KVER ?= $(shell uname -r)
KDIR := /lib/modules/$(KVER)/build/

all:
	make -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) modules

clean:
	make -C $(KDIR) M=$(CURDIR) clean
