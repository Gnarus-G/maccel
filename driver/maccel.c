#include "accel.h"
#include "dbg.h"
#include "linux/bits.h"
#include "linux/input-event-codes.h"
#include "linux/kern_levels.h"
#include "linux/ktime.h"
#include "linux/mod_devicetable.h"
#include "linux/printk.h"
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

static bool maccel_filter(struct input_handle *handle, unsigned int type,
                          unsigned int code, int value) {
  static int x_value = 0;
  static int y_value = 0;

  printk(KERN_INFO pr_fmt("Looking for: type %d, code %d (x) or code %d (y)"),
         EV_REL, REL_X, REL_Y);
  printk(KERN_INFO pr_fmt("Filter: type %d, code %d"), type, code);

  /* if (type == EV_REL) { */
  /*   AccelResult accelerated = accelerate(x_value, y_value); */
  /**/
  /*   if (code == REL_X) { */
  /*     x_value = value; */
  /*     input_event(handle->dev, type, code, accelerated.x); */
  /*   } */
  /**/
  /*   if (code == REL_Y) { */
  /*     y_value = value; */
  /*     input_event(handle->dev, type, code, accelerated.y); */
  /*   } */
  /**/
  /*   return false; // since we handled this skip the following event handlers.
   */
  /* } */

  return false;
}

static void maccel_event(struct input_handle *handle, unsigned int type,
                         unsigned int code, int value) {

  /* printk(KERN_INFO pr_fmt("Event: type %d, code %d"), type, code); */
}

static bool maccel_match(struct input_handler *handler, struct input_dev *dev) {
  struct hid_device *hdev = to_hid_device(dev->dev.parent);
  dbg("maccel handler found a mouse: %d", hdev->type == HID_TYPE_USBMOUSE);
  return hdev->type == HID_TYPE_USBMOUSE;
}

static int maccel_connect(struct input_handler *handler, struct input_dev *dev,
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

  printk(KERN_INFO pr_fmt("maccel handler connecting device: %s (%s at %s)"),
         dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");

  return 0;
}

static void maccel_disconnect(struct input_handle *handle) {
  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(handle);
}

static const struct input_device_id my_ids[] = {
    {.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
     .evbit = {BIT_MASK(EV_REL)}}, // Match all relative pointer values
    {},
};

MODULE_DEVICE_TABLE(input, my_ids);

static struct input_handler maccel_handler = {.event = maccel_event,
                                              .filter = maccel_filter,
                                              .connect = maccel_connect,
                                              .disconnect = maccel_disconnect,
                                              .name = "maccel",
                                              .id_table = my_ids,
                                              .match = maccel_match};

static int __init my_init(void) {
  return input_register_handler(&maccel_handler);
}

static void __exit my_exit(void) { input_unregister_handler(&maccel_handler); }

module_init(my_init);
module_exit(my_exit);
