obj-m += leetmouse.o
leetmouse-objs := usbmouse.o accel.o util.o

ccflags-y += -mhard-float -mpreferred-stack-boundary=4

all:
	cp -n config.sample.h config.h
