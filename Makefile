DRIVERDIR?=`pwd`/driver
MODULEDIR?=/lib/modules/`uname -r`/kernel/drivers/usb

build:
	$(MAKE) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" -C $(DRIVERDIR)

build_debug: EXTRA_CFLAGS = -g -DDEBUG
build_debug: build

test: 
	$(MAKE) -C $(DRIVERDIR) test
test_debug:
	$(MAKE) -C $(DRIVERDIR) test_debug

install_debug: build_debug install

install: build
	@sudo insmod $(DRIVERDIR)/maccel.ko;

	@mkdir -p $(MODULEDIR)
	@sudo cp -v $(DRIVERDIR)/*.ko $(MODULEDIR);
	@sudo chown -v root:root $(MODULEDIR)/*.ko;
	sudo groupadd -f maccel;
	sudo depmod; 
	sudo chown -v :maccel /sys/module/maccel/parameters/* /var/opt/maccel/resets/* /dev/maccel;
	sudo chmod g+w /var/opt/maccel/resets/*;
	sudo chmod g+r /dev/maccel;
	ls -l /sys/module/maccel/parameters/*

uninstall: clean
	@sudo rm -fv $(MODULEDIR)/maccel.ko
	@sudo rmmod maccel

reinstall: uninstall
	@sudo make install

reinstall_debug: uninstall
	@sudo make install_debug

build_cli:
	cargo build --release --manifest-path=cli/Cargo.toml

install_cli: build_cli
	sudo install -m 755 `pwd`/cli/target/release/maccel /usr/local/bin/maccel

uninstall_cli:
	@sudo rm -f /usr/local/bin/maccel

udev_install: install_cli
	sudo install -m 644 -v -D `pwd`/udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
	sudo install -m 755 -v -D `pwd`/udev_rules/maccel_param_ownership_and_resets /usr/lib/udev/maccel_param_ownership_and_resets 

udev_uninstall:
	@sudo rm -f /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
	sudo udevadm control --reload-rules

udev_trigger:
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02

clean:
	@rm -rf src pkg maccel maccel*.zst maccel-dkms*.log*
	$(MAKE) -C $(DRIVERDIR) clean
