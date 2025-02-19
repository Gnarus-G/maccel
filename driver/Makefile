obj-m += maccel.o

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

override EXTRA_CFLAGS += -DFIXEDPT_BITS=32

ifneq ($(CC),clang)
	CC=gcc
else
	export LLVM=1
endif

build:
	$(MAKE) CC=$(CC) EXTRA_CFLAGS='$(EXTRA_CFLAGS)' -C $(KDIR) M=$(CURDIR)

build_debug: EXTRA_CFLAGS += -g -DDEBUG
build_debug: build

clean:
		$(MAKE) -C $(KDIR) M=$(CURDIR) clean

test_debug: **/*.test.c
	@mkdir -p tests/snapshots
	@sh tests/run_tests.sh -DDEBUG

test: **/*.test.c
	@mkdir -p tests/snapshots
	@sh tests/run_tests.sh $(DEBUG)
