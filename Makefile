obj-m += leetmouse.o
leetmouse-objs := usbmouse.o accel.o util.o

ccflags-y += -mhard-float -mpreferred-stack-boundary=4

.PHONY: all clean

all:
	cp -n config.sample.h config.h
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
