#!/bin/sh

BRANCH=${BRANCH:-""}
DEBUG=${DEBUG:-0}
BUILD_CLI_FROM_SOURCE=${BUILD_CLI_FROM_SOURCE:-0}

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

  if [ -n "$BRANCH" ]; then
    print_bold "Will do an install, using the branch: $BRANCH\n"
    git clone --depth 1 --no-single-branch https://github.com/Gnarus-G/maccel.git .
    git switch $BRANCH
  else
    git clone --depth 1 https://github.com/Gnarus-G/maccel.git .
  fi
}

version_update_warning() {
  if [ -z "$CURR_VERSION" ]; then
    return
  fi

  MARKER_VERSION="0.4.0"

  if [ "$CURR_VERSION" = "$MARKER_VERSION" ]; then
    return
  fi

  if [[ "$CURR_VERSION" < "$MARKER_VERSION" ]]; then
    print_yellow $(print_bold "ATTENTION!")
    printf "\n\n"

    print_yellow "The precision for the processed values has been updated since version '$CURR_VERSION';\n"
    EMPHASIS=$(print_bold "MUST re-enter your parameter values in maccel")
    print_yellow "This means that you $EMPHASIS.\n"
    print_yellow "Otherwise your curve and mouse movement won't behave as expected.\n"

    printf "\nHere were your values as maccel understands them in '$CURR_VERSION':\n"

    print_bold "SENS MULT:  "
    maccel get param sens-mult

    print_bold "ACCEL:      "
    maccel get param accel

    print_bold "OFFSET:     "
    maccel get param offset

    print_bold "OUTPUT CAP: "
    maccel get param output-cap
  fi
}

install_udev_rules() {
  make udev_uninstall
  make udev_install
}

install_driver_dkms() {
  dkms_version=$(cat PKGBUILD | grep "pkgver=" | grep -oP '\d.\d.\d')

  ! sudo rmmod maccel 2>/dev/null; # It's obviously okay if this fails

  # Uninstall if this version already exists
  test -n "$(sudo dkms status maccel/$dkms_version)" && {
    sudo dkms remove maccel/$dkms_version
  }

  # Install Driver 
  install -Dm 644 "$(pwd)/dkms.conf" "/usr/src/maccel-${dkms_version}/dkms.conf"
  
  DEBUG_CFLAGS=""
  if [ $DEBUG -eq 1 ]; then
    print_bold "Debug build enabled\n"
    DEBUG_CFLAGS="-g -DDEBUG"
  fi

  # Set name and version
  sudo sed -e "s/@_PKGNAME@/maccel/" \
          -e "s/@PKGVER@/${dkms_version}/" \
          -e "s/@EXTRA_CFLAGS@/'${DEBUG_CFLAGS}'/" \
          -i "/usr/src/maccel-${dkms_version}/dkms.conf"
  
  sudo cp -r "$(pwd)/driver/." "/usr/src/maccel-${dkms_version}/"

  sudo dkms install --force "maccel/${dkms_version}"

  # Note(Gnarus):
  # This wouldn't ok in the .install file as noted in https://wiki.archlinux.org/title/DKMS_package_guidelines#Module_loading_automatically_in_.install
  # But I think it's ok here.
  sudo modprobe maccel
}

install_cli() {
  if [ $(getconf LONG_BIT) -lt 64 ]; then
    BUILD_CLI_FROM_SOURCE=1
  fi

  if [ $BUILD_CLI_FROM_SOURCE -eq 1 ]; then
    export RUSTUP_TOOLCHAIN=stable
    cargo build --bin maccel --release
    sudo install -m 755 `pwd`/target/release/maccel /usr/local/bin/maccel
  else
    print_bold "Preparing to download and install the CLI tool...\n"
    printf "If you want to build the CLI tool from source, then next time run: \n"
    print_bold "  curl -fsSL https://maccel.org/install.sh | sudo BUILD_CLI_FROM_SOURCE=1 sh \n"
    curl -fsSL https://github.com/Gnarus-G/maccel/releases/download/v$VERSION/maccel-cli.tar.gz -o maccel-cli.tar.gz
    tar -zxvf maccel-cli.tar.gz maccel_v$VERSION/maccel
    mkdir -p bin
    sudo install -m 755 -v -D maccel_v$VERSION/maccel* bin/
    sudo ln -vfs $(pwd)/bin/maccel* /usr/local/bin/
  fi

  sudo groupadd -f maccel
}

# ---- Install Process ----

ATTENTION=$(version_update_warning)

underline_start
print_bold "\nFetching the maccel github repo"
underline_end

setup_dirs

underline_start
print_bold "\nInstalling udev rules..."
underline_end

install_udev_rules

underline_start
print_bold "\nInstalling the driver (kernel module)"
underline_end

install_driver_dkms

underline_start
print_bold "\nInstalling the CLI"
underline_end

install_cli

print_bold $(print_green "[Recommended]")
print_bold ' Add yourself to the "maccel" group\n'
print_bold $(print_green "[Recommended]")
print_bold ' usermod -aG maccel $USER\n'

if [ -n "$ATTENTION" ]; then
  printf "\n$ATTENTION\n"
fi
