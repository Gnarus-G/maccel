MOD_NAME = maccel

obj-m += $(MOD_NAME).o

ccflags-y += -msse -mhard-float

CC=gcc
KDIR=/lib/modules/`uname -r`/build

default: 
	$(MAKE) CC=$(CC) -C $(KDIR) M=$$PWD

install:
	sudo insmod $(MOD_NAME).ko

uninstall:
	sudo rmmod $(MOD_NAME)

clean:
	rm -rf .*.cmd *.ko *.mod *.mod.* *.symvers *.order *.o
