A fork of the Linux USB mouse driver with acceleration.

It is based on Povohat's mouse driver for Windows: http://accel.drok-radnik.com/old.html

### Installation

Step 1: Clone this repository, copy config.sample.h to config.h and edit it in your favorite text editor. Change the defines at the top of config.h to match your desired acceleration settings.

The acceleration options are the same as those in Povohat's driver:
http://accel.drok-radnik.com/old.html

Step 2: Build and install the driver.
```
make clean && make
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
A "bind.sh" script with an explanation in the comments on how to find your mouse is available. This might speed up binding to this driver.
Please note: This is about to change in the future (auto-binding)

License: GPL

TODO:
* External interface for pushing acceleration parameters
* Easier installation & bind
* Feature parity with Povohat's mouse driver

Tested mice:
* (EricSchles) Microsoft Intellimouse Explorer 3.0
* (EricSchles) Logitech MX518
* SteelSeries Kana
* SteelSeries Rival 110
* SteelSeries Rival 600/650

Tested OS
* (EricSchles) Ubuntu 14.04
* Arch
* Manjaro

Feel free to open an issue if you're having any problems with the driver.
