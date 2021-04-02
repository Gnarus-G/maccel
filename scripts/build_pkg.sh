#!/bin/sh

TEMP_DIR=$(mktemp --suffix="_pkg_build_tmp" -d)
ROOT="$(git rev-parse --show-toplevel)"

# Read current version
VERSION=$(cat "${ROOT}/Makefile" | grep -oP "(?<=DKMS_VER\?=)[0-9\.]+")
SRC_NAME=leetmouse-$VERSION

# ########## Create output director
mkdir -p "${ROOT}/pkg/build"

# ########## Creates a compressed file for releases on github, mkpkg etc
# Tar, copy, untar, tar.xz: I know, this is stupid, but atleast I can skip all git ignored files with that method and control the base-folder in the tar.xz file
tar --exclude-vcs --exclude-vcs-ignores -cf $TEMP_DIR/tmp.tar -C "${ROOT}" .

cd $TEMP_DIR
rm -rf $SRC_NAME > /dev/null
mkdir -p $SRC_NAME
tar -xf tmp.tar -C $SRC_NAME
tar -cJf $SRC_NAME.tar.xz $SRC_NAME

# ########## Move compressed file to build folder
cd ${ROOT}/pkg/
mv $TEMP_DIR/$SRC_NAME.tar.xz "${ROOT}/pkg/build/"
