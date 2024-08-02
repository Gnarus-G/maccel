#include "accel.h"
#include "dbg.h"
#include "linux/ktime.h"
#include "params.h"
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver");

#define USB_VENDOR_ID_RAZER 0x1532
#define USB_VENDOR_ID_RAZER_VIPER 0x0078

static AccelResult inline accelerate(s8 x, s8 y) {
  static ktime_t last;
  static u64 last_ms = 1;

  ktime_t now = ktime_get();
  u64 ms = ktime_to_ms(now - last);

  last = now;

  if (ms < 1) { // ensure no less than 1ms
    ms = last_ms;
  }

  last_ms = ms;

  if (ms > 100) { // rounding dow to 100 ms
    ms = 100;
  }

  return f_accelerate(x, y, ms, PARAM_SENS_MULT, PARAM_ACCEL, PARAM_OFFSET,
                      PARAM_OUTPUT_CAP);
}

static bool my_input_filter(struct input_handle *handle, unsigned int type,
                            unsigned int code, int value) {
  static int x_value = 0;
  static int y_value = 0;

  if (type == EV_REL) {
    AccelResult accelerated = accelerate(x_value, y_value);

    if (code == REL_X) {
      x_value = value;
      input_event(handle->dev, type, code, accelerated.x);
    }

    if (code == REL_Y) {
      y_value = value;
      input_event(handle->dev, type, code, accelerated.y);
    }

    return true; // since we handled this skip the following event handlers.
  }

  return false;
}

static int my_input_connect(struct input_handler *handler,
                            struct input_dev *dev,
                            const struct input_device_id *id) {
  struct input_handle *handle;
  int error;

  handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!handle)
    return -ENOMEM;

  handle->dev = input_get_device(dev);
  handle->handler = handler;
  handle->name = "maccel";

  error = input_register_handle(handle);
  if (error) {
    kfree(handle);
    return error;
  }

  error = input_open_device(handle);
  if (error) {
    input_unregister_handle(handle);
    kfree(handle);
    return error;
  }

  return 0;
}

static void my_input_disconnect(struct input_handle *handle) {
  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(handle);
}

static const struct input_device_id my_ids[] = {
    {.driver_info = 1}, // Match all devices
    {},
};

MODULE_DEVICE_TABLE(input, my_ids);

static struct input_handler my_input_handler = {
    .filter = my_input_filter,
    .connect = my_input_connect,
    .disconnect = my_input_disconnect,
    .name = "maccel",
    .id_table = my_ids,
};

static int __init my_init(void) {
  return input_register_handler(&my_input_handler);
}

static void __exit my_exit(void) {
  input_unregister_handler(&my_input_handler);
}

module_init(my_init);
module_exit(my_exit);
