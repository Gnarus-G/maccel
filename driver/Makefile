obj-m += maccel.o

test:
	@mkdir -p tests/snapshots
	@gcc tests/test.c -o accel_test -g
	@./accel_test
	@rm accel_test
