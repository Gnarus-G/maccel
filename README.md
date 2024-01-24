# maccel

Linear mouse acceleration.

$$V = \frac{\sqrt{dx_0^2 + dy_0^2}}{i}$$

$$dx_f = dx_0 * (1 + aV)$$

$$dy_f = dy_0 * (1 + aV)$$

Where $dx$ and $dy$ are the directional displacements read from the mouse,
$i$ is the polling interval of the mouse; this would be 1ms if the polling rate is 1000Hz,
and $a$ is the user provided
acceleration factor

## Install

Clone the repo, and run some make commands.

```sh
git clone https://github.com/Gnarus-G/maccel.git
cd maccel
```

```sh
cargo build --release --manifest-path=maccel-cli/Cargo.toml
make install
make udev_install
```

Optionally, it's recommended that you add yourself to the `maccel` group;
and remember to restart your session after you do so it takes effect.

Adding yourself to that `maccel` group allows you to run the maccel-cli
to set parameter values without `sudo`

### Uninstall

```sh
make uninstall
make udev_uninstall
```

## CLI Usage

```
CLI to control the paramters for the maccel driver, and manage mice bindings

Usage: maccel <COMMAND>

Commands:
  bind       Attach a device to the maccel driver
  bindall    Attach all detected mice to the maccel driver
  unbind     Detach a device from the maccel driver, reattach to the generic usbhid driver
  unbindall  Detach all detected mice from the maccel driver reattach them to the generic usbhid driver
  set        Set the value for a parameter of the maccel driver
  get        Get the value for a parameter of the maccel driver
  help       Print this message or the help of the given subcommand(s)

Options:
  -h, --help     Print help
  -V, --version  Print version
```

## References

- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/drivers/hid/usbhid/usbmouse.c
- https://www.kernel.org/doc/html/latest/input/index.html
- https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md
- https://github.com/Skyl3r/leetmouse/blob/master/driver/accel.c
- https://sourceforge.net/p/fixedptc/code/ci/default/tree/
