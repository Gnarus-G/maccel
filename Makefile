# Makefile structure blatently "stolen" from OpenRazer 3.0.0 and adapted for leetmouse

# DESTDIR is used to install into a different root directory
DESTDIR?=/
# Specify the kernel directory to use
KERNELDIR?=/lib/modules/$(shell uname -r)/build
# Need the absolute directory do the driver directory to build kernel modules
DRIVERDIR?=$(shell pwd)/driver

# Where kernel drivers are going to be installed
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/drivers/usb

DKMS_NAME?=leetmouse-driver
DKMS_VER?=0.9.0


.PHONY: driver

all: driver
clean: driver_clean

driver:
	@echo -e "\n::\033[32m Compiling leetmouse kernel module\033[0m"
	@echo "========================================"
	cp -n $(DRIVERDIR)/config.sample.h $(DRIVERDIR)/config.h
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules


driver_clean:
	@echo -e "\n::\033[32m Cleaning leetmouse kernel module\033[0m"
	@echo "========================================"
	$(MAKE) -C "$(KERNELDIR)" M="$(DRIVERDIR)" clean

# Install kernel modules and then update module dependencies
driver_install:
	@echo -e "\n::\033[34m Installing leetmouse kernel module\033[0m"
	@echo "====================================================="
	@cp -v $(DRIVERDIR)/*.ko $(DESTDIR)/$(MODULEDIR)
	@chown -v root:root $(DESTDIR)/$(MODULEDIR)/*.ko
	depmod

# Remove kernel modules
driver_uninstall:
	@echo -e "\n::\033[34m Uninstalling leetmouse kernel module\033[0m"
	@echo "====================================================="
	@rm -fv $(DESTDIR)/$(MODULEDIR)/leetmouse.ko

setup_dkms:
	@echo -e "\n::\033[34m Installing DKMS files\033[0m"
	@echo "====================================================="
	install -m 644 -v -D Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/Makefile
	install -m 644 -v -D install_files/dkms/dkms.conf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/dkms.conf
	install -m 755 -v -d driver $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver
	install -m 644 -v -D driver/Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/Makefile
	install -m 644 -v driver/*.c $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	rm -fv $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/*.mod.c

remove_dkms:
	@echo -e "\n::\033[34m Removing DKMS files\033[0m"
	@echo "====================================================="
	rm -rf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)

udev_install:
	@echo -e "\n::\033[34m Installing leetmouse udev rules\033[0m"
	@echo "====================================================="
	install -m 644 -v -D install_files/udev/99-leetmouse.rules $(DESTDIR)/usr/lib/udev/rules.d/99-leetmouse.rules
	install -m 755 -v -D install_files/udev/leetmouse_bind $(DESTDIR)/usr/lib/udev/leetmouse_bind
	install -m 755 -v -D install_files/udev/leetmouse_manage $(DESTDIR)/usr/lib/udev/leetmouse_manage

udev_trigger:
	@echo -e "\n::\033[34m Triggering new udev rules\033[0m"
	@echo "====================================================="
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02

udev_uninstall:
	@echo -e "\n::\033[34m Uninstalling leetmouse udev rules\033[0m"
	@echo "====================================================="
	rm -f $(DESTDIR)/usr/lib/udev/rules.d/99-leetmouse.rules $(DESTDIR)/usr/lib/udev/leetmouse_bind $(DESTDIR)/usr/lib/udev/leetmouse_manage
	udevadm control --reload-rules
	. $(DESTDIR)/usr/lib/udev/leetmouse_manage unbind_all
	rm -f $(DESTDIR)/usr/lib/udev/leetmouse_manage

install_i_know_what_i_am_doing: all driver_install udev_install udev_trigger
install: manual_install_msg ;

manual_install_msg:
	@echo "Please do not install the driver using this method. Use a distribution package as it tracks the files installed and can remove them afterwards. If you are 100% sure, you want to do this, find the correct target in the Makefile."
	@echo "Exiting."

uninstall: driver_uninstall udev_uninstall
