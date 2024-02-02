# maccel

Linear mouse acceleration.
![image](https://github.com/Gnarus-G/maccel/assets/37311893/f45bc4bc-f7a0-43b0-9e8c-b3f6fb958d4c)

## Acceleration Funcion

$$V = \frac{\sqrt{dx_0^2 + dy_0^2}}{i}$$

$$dx_f = dx_0 * (1 + aV)$$

$$dy_f = dy_0 * (1 + aV)$$

Where $dx$ and $dy$ are the directional displacements read from the mouse,
$i$ is the polling interval of the mouse; this would be 1ms if the polling rate is 1000Hz,
and $a$ is the user provided
acceleration factor

## Install

```sh
curl -fsSL https://raw.githubusercontent.com/Gnarus-G/maccel/main/install.sh | sh
```

## Uninstall

```sh
sh /opt/maccel/uninstall.sh
```

Or

```sh
curl -fsSL https://raw.githubusercontent.com/Gnarus-G/maccel/main/uninstall.sh | sh
```

## CLI Usage

```
CLI to control the paramters for the maccel driver, and manage mice bindings

Usage: maccel <COMMAND>

Commands:
  tui        Open the Terminal UI to manage the parameters and see a graph of the sensitivity
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

## Notes

One should disable the acceleration done by default in some distros, e.g. by `xset` or `libinput`.
[Full Guide](https://wiki.archlinux.org/title/Mouse_acceleration#Disabling_mouse_acceleration)

Here is [Breakdown of why and how I ended up making this](https://www.bytin.tech/blog/maccel/)

## References

- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/drivers/hid/usbhid/usbmouse.c
- https://www.kernel.org/doc/html/latest/input/index.html
- https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md
- https://github.com/Skyl3r/leetmouse/blob/master/driver/accel.c
- https://sourceforge.net/p/fixedptc/code/ci/default/tree/
