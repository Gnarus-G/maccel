pkgname=maccel-dkms
_pkgname="maccel"
pkgver=0.3.1
pkgrel=1
pkgdesc="Mouse acceleration driver and kernel module for Linux."
arch=("x86_64")
url="https://www.maccel.org/"
license=("GPL-2.0-or-later")

install=maccel.install
depends=("dkms")
makedepends=("git" "cargo")

source=("git+https://github.com/Gnarus-G/maccel.git")
sha256sums=("SKIP")

prepare() {
    export RUSTUP_TOOLCHAIN=stable
    
    platform="$(rustc -vV | sed -n 's/host: //p')"
    
    cargo fetch --locked --target "${platform}" --manifest-path="${srcdir}/maccel/cli/Cargo.toml"
    cargo fetch --locked --target "${platform}" --manifest-path="${srcdir}/maccel/cli/usbmouse/Cargo.toml"
}

build() {
    export RUSTUP_TOOLCHAIN=stable
    export CARGO_TARGET_DIR=target
    
    # Build the CLI
	  cargo build --profile=release-with-debug --frozen --all-features --manifest-path="${srcdir}/maccel/cli/Cargo.toml"
	  cargo build --profile=release-with-debug --frozen --all-features --manifest-path="${srcdir}/maccel/cli/usbmouse/Cargo.toml"
}

package() {
    # Add group
    install -Dm 644 "${srcdir}/maccel/maccel.sysusers" "${pkgdir}/usr/lib/sysusers.d/${_pkgname}.conf"

    # Install Driver
    install -Dm 644 "${srcdir}/maccel/dkms.conf" "${pkgdir}/usr/src/${_pkgname}-${pkgver}/dkms.conf"

    # Set name and version
    sed -e "s/@_PKGNAME@/${_pkgname}/" \
        -e "s/@PKGVER@/${pkgver}/" \
        -i "${pkgdir}/usr/src/${_pkgname}-${pkgver}/dkms.conf"
    
    cp -r "${srcdir}/maccel/driver/." "${pkgdir}/usr/src/${_pkgname}-${pkgver}/"

    # Install CLI
    install -Dm 755 "${srcdir}/target/release-with-debug/maccel" "${pkgdir}/usr/bin/maccel"

    # Install udev rules
    install -Dm 644 "${srcdir}/maccel/udev_rules/99-maccel.rules" "${pkgdir}/usr/lib/udev/rules.d/99-maccel.rules"
    install -Dm 755 "${srcdir}/maccel/udev_rules/maccel_param_ownership_and_resets" "${pkgdir}/usr/lib/udev/maccel_param_ownership_and_resets"
    
    # Install License
    install -Dm 644 "${srcdir}/maccel/LICENSE" "${pkgdir}/usr/share/licenses/${_pkgname}/LICENSE"
}
