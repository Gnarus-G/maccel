# MACCEL_BRANCH

set -e

bold_start() {
  printf "\e[1m"
}

bold_end() {
  printf "\e[22m"
}

print_bold() {
  bold_start
  printf "$1"
  bold_end
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

get_current_version(){
  if ! which maccel &>/dev/null; then
    return
  fi

  maccel -V | awk '{ print $2 }'
}

get_version() {
  wget -qO- https://github.com/Gnarus-G/maccel/releases/latest | grep -oP 'v\d+\.\d+\.\d+' | tail -n 1 | cut -c 2-
}

CURR_VERSION=$(get_current_version)
VERSION=$(get_version)

set -e

setup_dirs() {
  rm -rf /opt/maccel && mkdir -p /opt/maccel
  cd /opt/maccel

  if [[ -n $MACCEL_BRANCH ]]; then
    print_bold "Will do an install, using the branch: $MACCEL_BRANCH\n"
    git clone --depth 1 --no-single-branch https://github.com/Gnarus-G/maccel.git .
    git switch $MACCEL_BRANCH
  else
    git clone --depth 1 https://github.com/Gnarus-G/maccel.git .
  fi
}

version_update_warning() {
  if [[ -n $CURR_VERSION && "$CURR_VERSION" < "0.1.3" ]]; then
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

install_driver_dkms() {  
    # Install Driver 
    install -Dm 644 "$(pwd)/dkms.conf" "/usr/src/maccel-${VERSION}/dkms.conf"
    
    # Set name and version
    sudo sed -e "s/@_PKGNAME@/maccel/" \
         -e "s/@PKGVER@/${VERSION}/" \
         -i "/usr/src/maccel-${VERSION}/dkms.conf"
    
    sudo cp -r "$(pwd)/driver/." "/usr/src/maccel-${VERSION}/"

    sudo dkms install "maccel/${VERSION}"

    print_bold $(print_green "[Required]")
    print_bold ' Make sure to run `modprobe maccel` after install\n'
    print_bold $(print_green "[Required]")
    print_bold ' unless you have `modprobe_on_install` added to your dkms config.\n'
}

install_cli() {
  curl -fsSL https://github.com/Gnarus-G/maccel/releases/download/v$VERSION/maccel-cli.tar.gz -o maccel-cli.tar.gz
  tar -zxvf maccel-cli.tar.gz maccel_v$VERSION/maccel
  mkdir -p bin
  sudo install -m 755 -v -D maccel_v$VERSION/maccel* bin/
  sudo ln -vfs $(pwd)/bin/maccel* /usr/local/bin/

  sudo groupadd -f maccel
}

install_udev_rules() {
	sudo rm -f /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
  sudo install -m 644 -v -D $(pwd)/udev_rules/99-maccel.rules /usr/lib/udev/rules.d/99-maccel.rules
  sudo install -m 755 -v -D $(pwd)/udev_rules/maccel_param_ownership_and_resets /usr/lib/udev/maccel_param_ownership_and_resets 
}

trigger_udev_rules() {
  udevadm control --reload-rules
  udevadm trigger --subsystem-match=usb --subsystem-match=input --subsystem-match=hid --attr-match=bInterfaceClass=03 --attr-match=bInterfaceSubClass=01 --attr-match=bInterfaceProtocol=02
}

# ---- Install Process ----

ATTENTION=$(version_update_warning)

underline_start
print_bold "\nFetching the maccel github repo"
underline_end

setup_dirs

underline_start
print_bold "\nInstalling the driver (kernel module)"
underline_end

install_driver_dkms

underline_start
print_bold "\nInstalling the CLI"
underline_end

install_cli

underline_start
print_bold "\nInstalling udev rules..."
underline_end

install_udev_rules
trigger_udev_rules

print_bold $(print_green "[Recommended]")
print_bold ' Add yourself to the "maccel" group\n'
print_bold $(print_green "[Recommended]")
print_bold ' usermod -aG maccel $USER\n'

if [[ -n "$ATTENTION" ]]; then
  printf "\n$ATTENTION\n"
fi
