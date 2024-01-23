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
sudo make install
sudo make udev_install
```

Optionally, it's recommended that you add yourself to the `maccel` group;
and remember to restart your session after you do so it takes effect.

Adding yourself to that `maccel` group allows you to run the maccel-cli
to set parameter values without `sudo`

### Uninstall

```sh
sudo make uninstall
sudo make udev_uninstall
```

## Tips

### finding the bus id of a usb device

```sh
lsusb # to get the Bus and device
lsusb -t # to get the Port of the root_hub, hub, or the Port of the device within that hub
# as well as interface numbers (If)
```

At this point the bus id can be probably formed by using one the patterns:

- Bus-Port_device:Subdevice.If, eg 3-1.2:1.0
- Bus-Port_hub.Port_device:Subdevice.If, eg 3-1.2:1.0

```sh
# see all the mice bus ids in the paths liste with this command
grep 02 /sys/bus/usb/devices/*/bInterfaceProtocol
```

### unbinding usbhid

```sh
BUS_ID=... # fill in bus id
echo "$BUS_ID" > /sys/bus/usb/drivers/usbhid/unbind
```

## References

- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/drivers/hid/usbhid/usbmouse.c
- https://www.kernel.org/doc/html/latest/input/index.html
- https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md
- https://github.com/Skyl3r/leetmouse/blob/master/driver/accel.c
- https://sourceforge.net/p/fixedptc/code/ci/default/tree/
