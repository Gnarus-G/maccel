#!/bin/sh

set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT

mkdir -p "$tmp_dir/bin"
log_file="$tmp_dir/sudo.log"

cat >"$tmp_dir/bin/sudo" <<'EOF'
#!/bin/sh

if [ "$1" = "dkms" ] && [ "$2" = "status" ]; then
  printf '%s\n' \
    'maccel/0.5.9, 6.12.1-arch1-1, x86_64: installed' \
    'maccel/0.5.9, 6.13.1-arch1-1, x86_64: installed' \
    'maccel/0.5.10, 6.13.1-arch1-1, x86_64: built'
  exit 0
fi

printf '%s\n' "$*" >>"$MACCEL_TEST_LOG"
EOF

cat >"$tmp_dir/bin/ls" <<'EOF'
#!/bin/sh
exit 1
EOF

chmod +x "$tmp_dir/bin/sudo" "$tmp_dir/bin/ls"

PATH="$tmp_dir/bin" MACCEL_TEST_LOG="$log_file" \
  /bin/sh "$repo_root/uninstall.sh" <<'EOF' >/dev/null
y
EOF

expected="$tmp_dir/expected.log"
cat >"$expected" <<'EOF'
rmmod maccel
dkms remove maccel/0.5.9 --all
dkms remove maccel/0.5.10 --all
rm -vf /usr/lib/udev/rules.d/99-maccel.rules /usr/lib/udev/maccel_param_ownership_and_resets
udevadm control --reload-rules
EOF

diff -u "$expected" "$log_file"
