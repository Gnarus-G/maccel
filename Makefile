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
	@rm -fv $(DESTDIR)/$(MODULEDIR)/maccel.ko
	
udev_install:
	install -m 644 -v -D udev_rules/99-maccel.rules $(DESTDIR)/usr/lib/udev/rules.d/99-maccel.rules
	install -m 755 -v -D udev_rules/maccel_bind $(DESTDIR)/usr/lib/udev/maccel_bind
	install -m 755 -v -D udev_rules/maccel_manage $(DESTDIR)/usr/lib/udev/maccel_manage

udev_uninstall:
	@rm -f $(DESTDIR)/usr/lib/udev/rules.d/99-maccel.rules $(DESTDIR)/usr/lib/udev/maccel_bind
	udevadm control --reload-rules
	. $(DESTDIR)/usr/lib/udev/maccel_manage unbind_all
	@rm -f $(DESTDIR)/usr/lib/udev/maccel_manage

udev_trigger:
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02

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
