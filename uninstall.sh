MODULEDIR=/lib/modules/$(uname -r)/kernel/drivers/usb

get_current_version(){
  if ! which maccel &>/dev/null; then
    return
  fi

  maccel -V | awk '{ print $2 }'
}

CURR_VERSION=$(get_current_version)

delete_module() {
  sudo rmmod maccel
  sudo rm -vf $MODULEDIR/maccel.ko
}

delete_module_dkms() {
  sudo rmmod maccel
  sudo dkms remove maccel/${CURR_VERSION}
  sudo rm -rfv /usr/src/maccel-${CURR_VERSION}
}

udev_uninstall() {
  if [[ "$CURR_VERSION" < "0.1.5" ]]; then
    sudo maccel unbindall
  else
    sudo maccel driver unbindall
  fi

	sudo rm -vf /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
  sudo udevadm control --reload-rules
}

delete_everything() {
  sudo rm -vf $(which maccel)
  sudo rm -vrf /opt/maccel /var/opt/maccel
}

udev_uninstall
#delete_module
delete_module_dkms
delete_everything
