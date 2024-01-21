MOD_NAME = maccel

CC=gcc
DRIVERDIR?=`pwd`/driver
KDIR?=/lib/modules/`uname -r`/build

MODULEDIR?=/lib/modules/`uname -r`/kernel/drivers/usb

default: 
	$(MAKE) CC=$(CC) -C $(KDIR) M=$(DRIVERDIR)

install: default
	sudo insmod $(MOD_NAME).ko

uninstall:
	sudo rmmod $(MOD_NAME)

restart: uninstall install

driver_install: default
	@cp -v $(DRIVERDIR)/*.ko $(MODULEDIR)
	@chown -v root:root $(MODULEDIR)/*.ko
	depmod

driver_uninstall:
	@rm -fv $(DESTDIR)/$(MODULEDIR)/leetmouse.ko

clean:
	rm -rf .*.cmd *.ko *.mod *.mod.* *.symvers *.order *.o

# The following is only for me @Gnarus: To bind my mice.
# While I haven't setup udev rules
bind_death_adder:
	echo "3-3.3:1.0" > /sys/bus/usb/drivers/usbhid/unbind
	echo "3-3.3:1.0" > /sys/bus/usb/drivers/maccel/bind

unbind_death_adder:
	echo "3-3.3:1.0" > /sys/bus/usb/drivers/maccel/unbind
	echo "3-3.3:1.0" > /sys/bus/usb/drivers/usbhid/bind

bind_viper:
	echo "5-3:1.0" > /sys/bus/usb/drivers/usbhid/unbind
	echo "5-3:1.0" > /sys/bus/usb/drivers/maccel/bind

unbind_viper:
	echo "5-3:1.0" > /sys/bus/usb/drivers/maccel/unbind
	echo "5-3:1.0" > /sys/bus/usb/drivers/usbhid/bind
