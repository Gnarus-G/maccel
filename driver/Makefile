obj-m += maccel.o

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

build:
		$(MAKE) -C $(KDIR) M=$(PWD)

clean:
		$(MAKE) -C $(KDIR) M=$(PWD) clean

test:
	@mkdir -p tests/snapshots
	@gcc tests/test.c -o accel_test -g
	@./accel_test
	@rm accel_test
