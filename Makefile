MOD_NAME = maccel

obj-m += $(MOD_NAME).o

CC=gcc
KDIR=/lib/modules/`uname -r`/build

MODULEDIR?=/lib/modules/`uname -r`/kernel/drivers/usb

default: 
	$(MAKE) CC=$(CC) -C $(KDIR) M=$$PWD

install: default
	sudo insmod $(MOD_NAME).ko

uninstall:
	sudo rmmod $(MOD_NAME)

restart: uninstall install

driver_install: default
	@cp -v $$PWD/*.ko $(MODULEDIR)
	@chown -v root:root $(MODULEDIR)/*.ko
	depmod

driver_uninstall:
	@rm -fv $(DESTDIR)/$(MODULEDIR)/leetmouse.ko

clean:
	rm -rf .*.cmd *.ko *.mod *.mod.* *.symvers *.order *.o
