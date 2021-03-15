#!/bin/sh

# Office PC
#DEVICES=( "3-1.2:1.1" )
# Home PC
#DEVICES=( "5-1.1:1.1" )
DEVICES=( "5-1.1:1.1" )

# How to get the device id
# Identify your mouse via 'lsusb'
#   Bus 003 Device 004: ID 1038:1729 SteelSeries ApS
# So your mouse is on Bus 3 and has the id 4

# Now dive in deeper with 'lsusb -t' and check out "Bus 3" in detail
#/:  Bus 03.Port 1: Dev 1, Class=root_hub, Driver=ehci-pci/2p, 480M
#    |__ Port 1: Dev 2, If 0, Class=Hub, Driver=hub/8p, 480M
#        |__ Port 1: Dev 3, If 0, Class=Human Interface Device, Driver=usbhid, 1.5M
#        |__ Port 1: Dev 3, If 1, Class=Human Interface Device, Driver=usbhid, 1.5M
#        |__ Port 2: Dev 4, If 0, Class=Human Interface Device, Driver=usbhid, 12M
#        |__ Port 2: Dev 4, If 1, Class=Human Interface Device, Driver=usbhid, 12M

# The node with 'Dev 4' tells you, your mouse is on Bus 3, Port 1, Port 2

# Now do a sanity check with 'ls /sys/bus/usb/devices'
#1-0:1.0  1-1.3      1-1.5      1-1.5:1.1  2-0:1.0  3-1    3-1.1:1.0  3-1.2      3-1.2:1.1  4-0:1.0  6-0:1.0  usb2  usb4  usb6
#1-1      1-1.3:1.0  1-1.5:1.0  1-1:1.0    3-0:1.0  3-1.1  3-1.1:1.1  3-1.2:1.0  3-1:1.0    5-0:1.0  usb1     usb3  usb5

# -> Select the matching device with USB 1.0 speed. Here, it is 3-1.2:1.0 (Bus3-Port1.Port2:USBSpeed)

for DEVICE in "${DEVICES[@]}"; do
    echo "Rebinding $DEVICE"
    echo -n "$DEVICE" > /sys/bus/usb/drivers/usbhid/unbind
    echo -n "$DEVICE" > /sys/bus/usb/drivers/leetmouse/bind
done
