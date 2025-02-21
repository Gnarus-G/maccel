#!/bin/sh

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

delete_module_dkms() {
  sudo rmmod maccel

  if test -n "$(ls /var/lib/pacman/local/maccel*)"; then
    sudo pacman -R maccel-dkms
    sudo pacman -R maccel-dkms-debug
  fi

  maccel_dkms_status=$(sudo dkms status maccel | grep 'maccel')
  if [ -n "$maccel_dkms_status" ]; then
    curr_dkms_vesions=$(echo $maccel_dkms_status | grep -oP '\d.\d.\d')
    echo $curr_dkms_vesions | xargs -I {} sudo dkms remove maccel/{}
  fi

}

udev_uninstall() {
  sudo rm -vf /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
  sudo udevadm control --reload-rules
}

uninstall_cli() {
  sudo rm -vf $(which maccel)
}

delete_everything() {
  sudo groupdel maccel
  sudo rm -vrf /opt/maccel /var/opt/maccel /usr/src/maccel-*
  sudo find /usr/lib/modules /var/lib/dkms -name "*maccel*" | xargs sudo rm -rfv
}

run() {
  delete_module_dkms
  uninstall_cli
  udev_uninstall

  print_bold "$(print_yellow "Do you plan to reinstall? [y]/n\n")"
  print_bold "If not, enter n[no] to delete everything.\n"

  read choice

  if [ "$choice" = "n" ] || [ "$choice" = "no" ]; then
    delete_everything
  fi
}

run 2>/dev/null
