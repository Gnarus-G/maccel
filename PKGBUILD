pkgname=maccel-dkms
_pkgname="${pkgname%-*}"
pkgver=0.2.1
pkgrel=1
pkgdesc='Mouse acceleration driver and kernel module for Linux.'
arch=('x86_64')
url='https://www.maccel.org/'
license=('GPL-2.0-or-later')

depends=("curl" "git" "dkms")
install=${_pkgname}.install

source=("git+https://github.com/pxlsec/maccel")
sha256sums=('SKIP')

build() {
    # Build the CLI
    cd $srcdir/maccel
    make build_cli
}

package() {
    # Add group
    install -Dm644 ${srcdir}/maccel/maccel.sysusers ${pkgdir}/usr/lib/sysusers.d/${_pkgname}.conf

    # Install Driver
    install -Dm644 ${srcdir}/maccel/dkms.conf ${pkgdir}/usr/src/${_pkgname}-${pkgver}/dkms.conf

    # Set name and version
    sed -e "s/@_PKGNAME@/${_pkgname}/" \
        -e "s/@PKGVER@/${pkgver}/" \
        -i "${pkgdir}/usr/src/${_pkgname}-${pkgver}/dkms.conf"

    # Copy sources (including Makefile)
    cp -r ${srcdir}/maccel/driver/* ${pkgdir}/usr/src/${_pkgname}-${pkgver}/

    # Install CLI
    install -Dm 755 $srcdir/maccel/cli/target/release/maccel ${pkgdir}/usr/local/bin/maccel
    install -Dm 755 $srcdir/maccel/cli/usbmouse/target/release/maccel-driver-binder ${pkgdir}/usr/local/bin/maccel-driver-binder

    # Install udev rules
    install -Dm 644 ${srcdir}/maccel/udev_rules/99-maccel.rules ${pkgdir}/usr/lib/udev/rules.d/99-maccel.rules
    install -Dm 755 ${srcdir}/maccel/udev_rules/maccel_param_ownership_and_resets ${pkgdir}/usr/lib/udev/maccel_param_ownership_and_resets
    
    # Install License
    install -Dm644 ${srcdir}/maccel/LICENSE ${pkgdir}/usr/share/licenses/${_pkgname}/LICENSE
}
