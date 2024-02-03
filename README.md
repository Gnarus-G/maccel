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

Make sure to have these dependencies installed on your machine:
`curl`, `git`, `make`, `gcc`, and the linux headers in `/lib/modules/`

```sh
curl -fsSL https://www.maccel.org/install.sh | sudo sh
```

## Uninstall

```sh
sh /opt/maccel/uninstall.sh
```

Or

```sh
curl -fsSL https://www.maccel.org/uninstall.sh | sudo sh
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

## Troubleshooting Install

### gcc

The version matters, it must match the version with which the kernel was built.

For example you might encounter such an error:

![image](https://github.com/Gnarus-G/maccel/assets/37311893/6147e20a-a132-4132-a45e-2af3dc035552)

And you'll have to find a version of `gcc` that matches. This will be more or less annoying
depending on your distro and/or how familiar you are with it.

### linux headers

You want to make sure that `/lib/modules/` is not empty. For example mine looks like this:

```
total 0
drwxr-xr-x 1 root root    114 Jan 29 17:59 .
drwxr-xr-x 1 root root 159552 Jan 29 22:39 ..
drwxr-xr-x 1 root root     10 Jan 29 17:59 6.6.14-1-lts
drwxr-xr-x 1 root root     12 Jan 29 17:59 6.7.0-zen3-1-zen
drwxr-xr-x 1 root root    494 Jan 29 17:59 6.7.2-arch1-1
drwxr-xr-x 1 root root    494 Jan 31 21:54 6.7.2-zen1-1-zen
```

You want to find headers that match your kernel as represented by

```
uname -r
```

On an arch based distro you list the available headers with

```
sudo pacman -Ss linux headers
```
