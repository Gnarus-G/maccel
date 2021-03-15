obj-m += leetmouse.o
leetmouse-objs := usbmouse.o accel.o

ccflags-y += -msse -mpreferred-stack-boundary=4

.PHONY: all clean

all:
	cp -n config.sample.h config.h
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
