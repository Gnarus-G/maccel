#!/bin/sh

ROOT="$(git rev-parse --show-toplevel)"

# Read current version
VERSION=$(cat "${ROOT}/Makefile" | grep -oP "(?<=DKMS_VER\?=)[0-9\.]+")
SRC_NAME=leetmouse-$VERSION

SRC=$SRC_NAME.tar.xz

if [ "$1" = "aur" ]; then
    SRC='https://github.com/systemofapwne/leetmouse/releases/download/v$pkgver/leetmouse-$pkgver.tar.xz'
fi

# Clear the build folder from old releases
rm -rf $ROOT/pkg/build/

# Create new package file
. $ROOT/scripts/build_pkg.sh


# ########## Generate PKGBUILD for Arch based systems
HASH=$(sha256sum "${ROOT}/pkg/build/$SRC_NAME.tar.xz" | awk '{ print $1 }')
cp -f "${ROOT}/pkg/PKGBUILD.template" "${ROOT}/pkg/build/PKGBUILD"
sed -i 's|'__VERSION__'|'$VERSION'|' "${ROOT}/pkg/build/PKGBUILD"
sed -i 's|'__HASH__'|'$HASH'|' "${ROOT}/pkg/build/PKGBUILD"
sed -i 's|'__SRC__'|'$SRC'|' "${ROOT}/pkg/build/PKGBUILD"
