# maccel

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
# see all the usb devices' as their bus ids
ls /sys/bus/usb/devices
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
