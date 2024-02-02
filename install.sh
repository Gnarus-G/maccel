git clone https://github.com/Gnarus-G/maccel.git
cd maccel

# Install the driver
make install

# Install the cli
cargo install --path maccel-cli

# Install the udev rules
make udev_install
