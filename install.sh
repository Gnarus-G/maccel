set -ex

setup_dirs() {
  rm -rf /opt/maccel && mkdir -p /opt/maccel
  cd /opt/maccel
  git clone --depth 1 https://github.com/Gnarus-G/maccel.git .

  mkdir -p bin
}

install_driver() {
  make uninstall || true
  make install
}


install_cli() {
  export VERSION=$(wget -qO- https://github.com/Gnarus-G/maccel/releases/latest | grep -oP 'v\d+\.\d+\.\d+' | tail -n 1);
  curl -fsSL https://github.com/Gnarus-G/maccel/releases/download/$VERSION/maccel-cli.tar.gz -o maccel-cli.tar.gz
  tar -zxvf maccel-cli.tar.gz maccel_$VERSION/maccel 
  sudo install -m 755 maccel_$VERSION/maccel bin
  sudo ln -s bin/maccel /usr/local/bin/maccel
}

install_udev_rules() {
	sudo install -m 644 -v -D `pwd`/udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
	sudo install -m 755 -v -D `pwd`/udev_rules/maccel_bind /usr/lib/udev/maccel_bind
}

trigger_udev_rules() {
	udevadm control --reload-rules
	udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02
}


# Run the installation
setup_dirs
install_driver
install_udev_rules

trigger_udev_rules

install_cli

