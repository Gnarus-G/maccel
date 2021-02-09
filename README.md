A fork of the Linux USB mouse driver with acceleration.

It is based on Povohat's mouse driver for Windows: http://accel.drok-radnik.com/old.html

### Installation

Step 1: Clone this repository and open leetmouse.h in your favorite text editor. Change the defines at the top of leetmouse.h to match your mouse's polling rate and your desired acceleration settings.

If you don't know what your mouse's polling rate is, you can follow this link:
https://wiki.archlinux.org/index.php/Mouse_polling_rate

The acceleration options are the same as those in Povohat's driver:
http://accel.drok-radnik.com/old.html

Step 2: Build and install the driver.
```
make
sudo rmmod leetmouse # removes the old version of this driver
sudo insmod leetmouse.ko
```

Step 3: Do a bunch of kernel module bind/unbind magic: https://lwn.net/Articles/143397/

This is what I had to do to get it to work. You will have to change "2-2:1.0" to whatever your mouse actually is and run these in a root shell.
```
sudo su
echo -n "2-2:1.0" > /sys/bus/usb/drivers/usbhid/unbind
echo -n "2-2:1.0" > /sys/bus/usb/drivers/leetmouse/bind
```

License: GPL

TODO:
* Feature parity with Povohat's mouse driver
* Easier installation

I've only tested the driver on Ubuntu 14.04 with a Microsoft Intellimouse Explorer 3.0 and a Logitech MX518. Feel free to open an issue if you're having any problems with the driver.
