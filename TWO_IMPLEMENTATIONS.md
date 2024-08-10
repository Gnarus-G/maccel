## Two Drivers

maccel has two drivers. One, the legacy, driver is a usb mouse driver that follows a very simple
process for getting mouse inputs and reporting the modifying inputs. The limitation of this driver
is that it might not work for some mice, because it binds your devices away from linux input system which
already supports your mouse. See [this issue](https://github.com/Gnarus-G/maccel/issues/9).

The recommended (default) driver is an input handler kernel module, such that it applies acceleration
on the mouse inputs that have already been properly parsed by the linux kernel.

## Difference

The old driver will either not support your device at all, or not support some extra functions (keys)
that your mouse has.

The new driver has general compatibility with the mice that linux already supports. 
In exchange it has 'more' lag than the old driver. Inconsequentially more. See the plot.

![Screenshot_2024-08-10_02-47-37](https://github.com/user-attachments/assets/def3a9d0-2bf3-401e-960c-b299592d1658)

We're talking microseconds here, so you have no reason to use the old driver.
If, for some reason, you want to use the old driver and it supports your mouse. You can bind to it like so:
```sh
sudo maccel bindall --install # --install makes it so it persists across boots
```
and unbind like so:
```sh
sudo maccel unbindall --uninstall # --uninstall makes it so it no longer rebinds across boots
```

### WARNING
I will most likely delete the old driver someday to simplify things. There is probably no point to keeping it.
