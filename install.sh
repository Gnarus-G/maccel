# MACCEL_DEBUG_INSTALL=0

print_bold() {
  printf "\e[1m$1\e[22m"
}

print_yellow() {
  printf "\e[33m$1\e[0m"
}

print_green() {
  printf "\e[32m$1\e[0m"
}

underline_start() {
  printf "\e[4m"
}

underline_end() {
  printf "\e[24m\n"
}

set -e

setup_dirs() {
  rm -rf /opt/maccel && mkdir -p /opt/maccel
  cd /opt/maccel
  git clone --depth 1 https://github.com/Gnarus-G/maccel.git .
}

version_update_warning() {

  if ! which maccel &>/dev/null; then
    print_yellow "BAD"
    return
  fi

  CURR_VERSION=$(maccel -V | awk '{ print $2 }')

  if [[ "$CURR_VERSION" < "0.1.3" ]]; then
    print_yellow $(print_bold "ATTENTION!")
    printf "\n\n"

    print_yellow "The precision for the processed values has been updated since version '$CURR_VERSION';\n"
    EMPHASIS=$(print_bold "MUST re-enter your parameter values in maccel")
    print_yellow "This means that you $EMPHASIS.\n"
    print_yellow "Otherwise your curve and mouse movement won't behave as expected.\n"

    printf "\nHere were your values as maccel understands them in '$CURR_VERSION':\n"

    print_bold "SENS MULT:  "
    maccel get sens-mult

    print_bold "ACCEL:      "
    maccel get accel

    print_bold "OFFSET:     "
    maccel get offset

    print_bold "OUTPUT CAP: "
    maccel get output-cap
  fi
}

install_driver() {
  make uninstall || true
  if [ $MACCEL_DEBUG_INSTALL -eq 1 ]; then
    echo "Will do a debug install as requested, MACCEL_DEBUG_INSTALL=1"
    make debug_install
  else
    make install
  fi
}

install_cli() {
  export VERSION=$(wget -qO- https://github.com/Gnarus-G/maccel/releases/latest | grep -oP 'v\d+\.\d+\.\d+' | tail -n 1)
  curl -fsSL https://github.com/Gnarus-G/maccel/releases/download/$VERSION/maccel-cli.tar.gz -o maccel-cli.tar.gz
  tar -zxvf maccel-cli.tar.gz maccel_$VERSION/maccel
  sudo install -m 755 -v -D maccel_$VERSION/maccel bin/maccel
  sudo ln -vfs $(pwd)/bin/maccel /usr/local/bin/maccel
}

install_udev_rules() {
  sudo install -m 644 -v -D $(pwd)/udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
  sudo install -m 755 -v -D $(pwd)/udev_rules/maccel_bind /usr/lib/udev/maccel_bind
}

trigger_udev_rules() {
  udevadm control --reload-rules
  udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02
}

ATTENTION=$(version_update_warning)

underline_start
print_bold "\nFetching the maccel github repo"
underline_end

setup_dirs

underline_start
print_bold "\nInstalling the driver (kernel module)"
underline_end

install_driver

underline_start
print_bold "\nInstalling udev rules..."
underline_end

install_udev_rules
trigger_udev_rules

underline_start
print_bold "\nInstalling the CLI"
underline_end

install_cli

print_bold $(print_green "[Recommended]")
print_bold ' Add yourself to the "maccel" group\n'
print_bold $(print_green "[Recommended]")
print_bold ' usermod -aG maccel $$USER\n'

printf "\n$ATTENTION\n"
