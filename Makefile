MOD_NAME = maccel

obj-m += $(MOD_NAME).o

CC=gcc
KDIR=/lib/modules/`uname -r`/build

default: 
	$(MAKE) CC=$(CC) -C $(KDIR) M=$$PWD

install: default
	sudo insmod $(MOD_NAME).ko

uninstall:
	sudo rmmod $(MOD_NAME)

restart: uninstall install

clean:
	rm -rf .*.cmd *.ko *.mod *.mod.* *.symvers *.order *.o
