A fork of the Linux USB mouse driver with acceleration.

Change the defines at the top of leetmouse.c to match your mouse's polling rate and your desired acceleration.

How to install:
```
make
sudo insmod leetmouse.ko

# Then do a bunch of kernel module bind/unbind magic: https://lwn.net/Articles/143397/

# This is what I had to do to get it to work.
# You will have to change "2-2:1.0" to whatever your mouse actually is and run these in a root shell.
echo -n "2-2:1.0" > /sys/bus/usb/drivers/usbhid/unbind
echo -n "2-2:1.0" > /sys/bus/usb/drivers/leetmouse/bind
```

License: GPL

TODO:
* Feature parity with Povohat's mouse driver for Windows: http://accel.drok-radnik.com/old.html
* Easier installation
