#include "accel.h"
#include "linux/bits.h"
#include "linux/gfp_types.h"
#include "linux/input-event-codes.h"
#include "linux/kern_levels.h"
#include "linux/ktime.h"
#include "linux/mod_devicetable.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "params.h"
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver");

static AccelResult inline accelerate(int x, int y) {
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

struct input_dev *virtual_input_dev;

static bool maccel_filter(struct input_handle *handle, unsigned int type,

                          unsigned int code, int value) {
  bool is_mouse_move = type == EV_REL && (code == REL_X || code == REL_Y);

  if (is_mouse_move) {
    static int mouse_move[2] = {0};
    static short occupancy = 0;
    mouse_move[code] = value;
    occupancy++;

    if (occupancy == 2) {
      AccelResult accelerated = accelerate(mouse_move[0], mouse_move[1]);

      input_report_rel(virtual_input_dev, REL_X, accelerated.x);
      input_report_rel(virtual_input_dev, REL_Y, accelerated.y);
      input_sync(virtual_input_dev);

      /* printk(KERN_INFO "accel: (%d, %d) -> (%d, %d)", mouse_move[0], */
      /*        mouse_move[1], accelerated.x, accelerated.y); */

      // Reset REL movement cache.
      occupancy = 0;
      mouse_move[0] = 0;
      mouse_move[1] = 0;
    }

    return true; // so input system skips (filters out) this unaccelerated
                 // mouse input.
  }

  return false;
}

static bool maccel_match(struct input_handler *handler, struct input_dev *dev) {
  if (dev->dev.parent == NULL) { // Probably our virtual driver;
    return false;
  }
  struct hid_device *hdev = to_hid_device(dev->dev.parent);

  printk(KERN_INFO "maccel found a possible mouse: %s", hdev->name);
  /* printk(KERN_INFO "is it a mouse? %s", */
  /*        hdev->type == HID_TYPE_USBMOUSE ? "true" : "false"); */

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
  if (error)
    goto err_free_mem;

  error = input_open_device(handle);
  if (error)
    goto err_unregister_handle;

  printk(KERN_INFO pr_fmt("maccel connecting to device: %s (%s at %s)"),
         dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");

  return 0;

err_unregister_handle:
  input_unregister_handle(handle);

err_free_mem:
  kfree(handle);
  return error;
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

static struct input_handler maccel_handler = {.filter = maccel_filter,
                                              .connect = maccel_connect,
                                              .disconnect = maccel_disconnect,
                                              .name = "maccel",
                                              .id_table = my_ids,
                                              .match = maccel_match};

// Function to create the virtual input device
static int create_virtual_device(void) {
  int error;

  virtual_input_dev = input_allocate_device();
  if (!virtual_input_dev) {
    printk(KERN_ERR "Failed to allocate virtual input device\n");
    return -ENOMEM;
  }

  virtual_input_dev->name = "maccel [Virtual Mouse]";
  virtual_input_dev->id.bustype = BUS_USB;
  /* virtual_input_dev->id.vendor = 0x1234; */
  /* virtual_input_dev->id.product = 0x5678; */
  virtual_input_dev->id.version = 1;

  // Set the supported event types and codes for the virtual device
  set_bit(EV_KEY, virtual_input_dev->evbit);
  set_bit(BTN_LEFT, virtual_input_dev->keybit);
  set_bit(BTN_RIGHT, virtual_input_dev->keybit);

  set_bit(EV_REL, virtual_input_dev->evbit);
  set_bit(REL_X, virtual_input_dev->relbit);
  set_bit(REL_Y, virtual_input_dev->relbit);

  error = input_register_device(virtual_input_dev);
  if (error) {
    printk(KERN_ERR "Failed to register virtual input device\n");
    input_free_device(virtual_input_dev);
    return error;
  }

  return 0;
}

static int __init my_init(void) {
  int error;
  error = create_virtual_device();
  if (error) {
    return error;
  }
  return input_register_handler(&maccel_handler);
}

static void __exit my_exit(void) {
  input_unregister_handler(&maccel_handler);
  input_unregister_device(virtual_input_dev);
  input_free_device(virtual_input_dev);
}

module_init(my_init);
module_exit(my_exit);
