#include "./accelk.h"
#include "input_echo.h"
#include <linux/hid.h>

static struct input_dev *virtual_input_dev;

#define NONE_EVENT_VALUE 0
static int relative_axis_events[REL_CNT] = { // [x, y, ..., wheel, ...]
    NONE_EVENT_VALUE};

static bool maccel_filter(struct input_handle *handle, u32 type,

                          u32 code, int value) {
  /* printk(KERN_INFO "type %d, code %d, value %d", type, code, value); */

  switch (type) {
  case EV_REL: {
    dbg("EV_REL => code %d, value %d", code, value);
    relative_axis_events[code] = value;

    // So we can relay the original speed of the mouse movement to userspace
    // for visualization.
    input_cache[0] = relative_axis_events[REL_X];
    input_cache[1] = relative_axis_events[REL_Y];

    return true; // so input system skips (filters out) this unaccelerated
                 // mouse input.
  }
  case EV_SYN: {
    int x = relative_axis_events[REL_X];
    int y = relative_axis_events[REL_Y];
    if (x || y) {
      dbg("EV_SYN => code %d", code);

      AccelResult accelerated = accelerate(x, y);
      dbg("accel: (%d, %d) -> (%d, %d)", x, y, accelerated.x, accelerated.y);
      x = accelerated.x;
      y = accelerated.y;

      if (x) {
        input_report_rel(virtual_input_dev, REL_X, x);
      }
      if (y) {
        input_report_rel(virtual_input_dev, REL_Y, y);
      }

      relative_axis_events[REL_X] = NONE_EVENT_VALUE;
      relative_axis_events[REL_Y] = NONE_EVENT_VALUE;
    }

    for (u32 code = REL_Z; code < REL_CNT; code++) {
      int value = relative_axis_events[code];
      if (value != NONE_EVENT_VALUE) {
        input_report_rel(virtual_input_dev, code, value);
        relative_axis_events[code] = NONE_EVENT_VALUE;
      }
    }

    input_sync(virtual_input_dev);
    return false;
  }

  default:
    return false;
  }
}

static bool maccel_match(struct input_handler *handler, struct input_dev *dev) {
  if (dev == virtual_input_dev)
    return false;
  if (!dev->dev.parent)
    return false;

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

struct input_handler maccel_handler = {.filter = maccel_filter,
                                       .connect = maccel_connect,
                                       .disconnect = maccel_disconnect,
                                       .name = "maccel",
                                       .id_table = my_ids,
                                       .match = maccel_match};
