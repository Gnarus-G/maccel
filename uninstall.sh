MODULEDIR=/lib/modules/`uname -r`/kernel/drivers/usb

delete_module() {
	sudo rmmod maccel
	sudo rm -vf $MODULEDIR/maccel.ko
}


udev_uninstall() {
	sudo maccel unbindall

	sudo rm -vf /usr/lib/udev/rules.d/99-maccel.rules /usr/lib/udev/maccel_bind
	sudo udevadm control --reload-rules
}

delete_everything() {
  sudo rm -vf $(which maccel)
  sudo rm -vrf /opt/maccel /var/opt/maccel
}

delete_module
udev_uninstall
delete_everything
