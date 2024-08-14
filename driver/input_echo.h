#ifndef _INPUT_ECHO_
#define _INPUT_ECHO_

#include "./accel.h"
#include "linux/cdev.h"

/**
 * Cache of the last [REL_X, REL_Y] values to report to userspace
 * on read.
 */
static int input_cache[2] = {0};

static struct cdev device;
static struct class *device_class;
static dev_t device_number;

/*
 * Convert an int into an array of four bytes, in big endian (MSB first)
 */
static void int_to_bytes(int num, char bytes[4]) {
  bytes[0] = (num >> 24) & 0xFF; // Most significant byte
  bytes[1] = (num >> 16) & 0xFF;
  bytes[2] = (num >> 8) & 0xFF;
  bytes[3] = num & 0xFF; // Least significant byte
}

static ssize_t read(struct file *f, char __user *user_buffer, size_t size,
                    loff_t *offset) {
  int x = input_cache[0];
  int y = input_cache[1];
  fixedpt speed = input_speed(fixedpt_fromint(x), fixedpt_fromint(y), 1);

  signed char be_bytes_for_int[4] = {0};
  int_to_bytes(speed, be_bytes_for_int);

  int err =
      copy_to_user(user_buffer, be_bytes_for_int, sizeof(be_bytes_for_int));
  if (err)
    return -EFAULT;

  return sizeof(be_bytes_for_int);
}

struct file_operations fops = {.owner = THIS_MODULE, .read = read};

static int create_char_device(void) {
  int err;
  err = alloc_chrdev_region(&device_number, 0, 1, "maccel");
  if (err)
    return -EIO;

  cdev_init(&device, &fops);
  device.owner = THIS_MODULE;

  cdev_add(&device, device_number, 1);

  device_class = class_create("maccel");

  if (IS_ERR(device_class)) {
    goto err_free_cdev;
  }

  device_create(device_class, NULL, device_number, NULL, "maccel");

  return 0;

err_free_cdev:
  cdev_del(&device);
  unregister_chrdev_region(device_number, 1);
  return -EIO;
}

static void destroy_char_device(void) {
  device_destroy(device_class, device_number);
  class_destroy(device_class);
  cdev_del(&device);
  unregister_chrdev_region(device_number, 1);
}

#endif // !_INPUT_ECHO_
