A modded version of the Linux USB mouse driver with acceleration.

How to install:
```
make
sudo insmod leetmouse.ko

# then do a bunch of kernel module bind/unbind magic like this:
echo -n "2-2:1.0" > /sys/bus/usb/drivers/usbhid/unbind
echo -n "2-2:1.0" > /sys/bus/usb/drivers/leetmouse/bind
```

License: GPL

TODO:
* Write better installation guide.
* Make acceleration configurable.

