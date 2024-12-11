ifneq ($(CC),clang)
	CC=gcc
else
	export LLVM=1
endif
DRIVERDIR?=`pwd`/driver
KDIR?=/lib/modules/`uname -r`/build

MODULEDIR?=/lib/modules/`uname -r`/kernel/drivers/usb

default: build

debug: EXTRA_CFLAGS := -DDEBUG
debug: default

build:
	$(MAKE) CC=$(CC) EXTRA_CFLAGS=$(EXTRA_CFLAGS) -C $(KDIR) M=$(DRIVERDIR)

debug_install: debug install

install: default
	@sudo insmod $(DRIVERDIR)/maccel.ko;

	@mkdir -p $(MODULEDIR)
	@sudo cp -v $(DRIVERDIR)/*.ko $(MODULEDIR);
	@sudo chown -v root:root $(MODULEDIR)/*.ko;
	sudo groupadd -f maccel;
	sudo depmod; 
	sudo chown -v :maccel /sys/module/maccel/parameters/* /dev/maccel;
	sudo chmod g+r /dev/maccel;
	ls -l /sys/module/maccel/parameters/*

uninstall: clean
	@sudo rm -fv $(MODULEDIR)/maccel.ko
	@sudo rmmod maccel

refresh: default uninstall
	@sudo make install

refresh-debug: default uninstall
	@sudo make debug_install

build_cli:
	cargo build --release --manifest-path=cli/Cargo.toml
	cargo build --release --manifest-path=cli/usbmouse/Cargo.toml

install_cli: build_cli
	sudo install -m 755 `pwd`/cli/target/release/maccel /usr/local/bin/maccel
	sudo install -m 755 `pwd`/cli/usbmouse/target/release/maccel-driver-binder /usr/local/bin/maccel-driver-binder

uninstall_cli:
	@sudo rm -f /usr/local/bin/maccel
	@sudo rm -f /usr/local/bin/maccel-driver-binder

udev_install: install_cli
	sudo install -m 644 -v -D `pwd`/udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
	sudo install -m 755 -v -D `pwd`/udev_rules/maccel_param_ownership_and_resets /usr/lib/udev/maccel_param_ownership_and_resets 

udev_uninstall:
	@sudo rm -f /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
	sudo udevadm control --reload-rules
	sudo /usr/local/bin/maccel-driver-binder unbindall

udev_trigger:
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02

clean:
	rm -rf $(DRIVERDIR)/.*.cmd $(DRIVERDIR)/*.ko $(DRIVERDIR)/*.mod $(DRIVERDIR)/*.mod.* $(DRIVERDIR)/*.symvers $(DRIVERDIR)/*.order $(DRIVERDIR)/*.o
	cargo clean --manifest-path=cli/Cargo.toml
	cargo clean --manifest-path=cli/usbmouse/Cargo.toml
