CC=gcc
DRIVERDIR?=`pwd`/driver
KDIR?=/lib/modules/`uname -r`/build

MODULEDIR?=/lib/modules/`uname -r`/kernel/drivers/usb

default: 
	$(MAKE) CC=$(CC) -C $(KDIR) M=$(DRIVERDIR)

install: default
	@cp -v $(DRIVERDIR)/*.ko $(MODULEDIR);
	@chown -v root:root $(MODULEDIR)/*.ko;
	@insmod $(MODULEDIR)/*.ko;
	groupadd -f maccel;
	depmod; 
	sudo chown -v root:maccel /sys/module/maccel/parameters/*;
	ls -l /sys/module/maccel/parameters/*
	@echo '[Recommended] Add yourself to the "maccel" group'
	@echo '[Recommended] usermod -aG maccel $$USER'

uninstall:
	sudo rmmod maccel

update: uninstall install 

sys_uninstall: default
	@rm -fv $(MODULEDIR)/maccel.ko
	
udev_install:
	install -m 644 -v -D udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
	install -m 755 -v -D udev_rules/maccel_bind /usr/lib/udev/maccel_bind
	install -m 755 -v -D udev_rules/maccel_manage /usr/lib/udev/maccel_manage

udev_uninstall:
	@rm -f /usr/lib/udev/rules.d/99-maccel.rules /usr/lib/udev/maccel_bind
	udevadm control --reload-rules
	. /usr/lib/udev/maccel_manage unbind_all
	@rm -f /usr/lib/udev/maccel_manage

udev_trigger:
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02

clean:
	rm -rf $(DRIVERDIR)/.*.cmd $(DRIVERDIR)/*.ko $(DRIVERDIR)/*.mod $(DRIVERDIR)/*.mod.* $(DRIVERDIR)/*.symvers $(DRIVERDIR)/*.order $(DRIVERDIR)/*.o

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
