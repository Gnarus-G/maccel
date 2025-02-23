obj-m += maccel.o

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

override EXTRA_CFLAGS += -DFIXEDPT_BITS=$(shell getconf LONG_BIT)

ifneq ($(CC),clang)
	CC=gcc
else
	export LLVM=1
endif

build:
	$(MAKE) CC=$(CC) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" -C $(KDIR) M=$(CURDIR)

build_debug: EXTRA_CFLAGS += -g -DDEBUG
build_debug: build

clean:
		$(MAKE) -C $(KDIR) M=$(CURDIR) clean

test_debug: EXTRA_CFLAGS += -g -DDEBUG
test_debug: test

test: **/*.test.c
	@mkdir -p tests/snapshots
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" TEST_NAME=$(name) sh tests/run_tests.sh
