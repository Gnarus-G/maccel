ifneq ($(KERNELRELEASE),)
	obj-m      := maccel.o
	ccflags-y += $(DRIVER_CFLAGS)
endif

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

DRIVER_CFLAGS ?= -DFIXEDPT_BITS=$(shell getconf LONG_BIT)

ifneq ($(CC),clang)
	CC=gcc
else
	export LLVM=1
endif

build:
	$(MAKE) CC=$(CC) -C $(KDIR) M=$(CURDIR)

build_debug: DRIVER_CFLAGS += -g -DDEBUG
build_debug: build

clean:
		$(MAKE) -C $(KDIR) M=$(CURDIR) clean

test_debug: DRIVER_CFLAGS += -g -DDEBUG
test_debug: test

test: **/*.test.c
	@mkdir -p tests/snapshots
	DRIVER_CFLAGS="$(DRIVER_CFLAGS)" TEST_NAME=$(name) sh tests/run_tests.sh
