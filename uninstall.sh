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
  sudo rmmod maccel 2>/dev/null || true

  if command -v pacman >/dev/null 2>&1; then
    for package in maccel-dkms maccel-dkms-debug; do
      if pacman -Qq "$package" >/dev/null 2>&1; then
        sudo pacman -R "$package"
      fi
    done
  fi

  maccel_dkms_status=$(sudo dkms status maccel 2>/dev/null || true)
  if [ -n "$maccel_dkms_status" ]; then
    seen_versions="|"
    while IFS= read -r status_line; do
      module_version=${status_line%%,*}
      module_version=${module_version%%:*}

      case "$module_version" in
        maccel/*)
          case "$seen_versions" in
            *"|$module_version|"*) continue ;;
          esac

          sudo dkms remove "$module_version" --all
          seen_versions="${seen_versions}${module_version}|"
          ;;
      esac
    done <<EOF
$maccel_dkms_status
EOF
  fi

}

udev_uninstall() {
  sudo rm -vf /usr/lib/udev/rules.d/99-maccel*.rules /usr/lib/udev/maccel_*
  sudo udevadm control --reload-rules
}

uninstall_cli() {
  maccel_path=$(command -v maccel || true)
  case "$maccel_path" in
    /usr/local/bin/maccel* | /opt/maccel/*) sudo rm -vf "$maccel_path" ;;
  esac
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

run
