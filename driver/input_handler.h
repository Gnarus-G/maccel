#include "./accel_k.h"
#include "linux/input.h"
#include "mouse_move.h"
#include <linux/hid.h>
#include <linux/version.h>

/*
 * Collect the events EV_REL REL_X and EV_REL REL_Y, once we have both then
 * we accelerate the (x, y) vector and set the EV_REL event's value
 * through the `value_ptr` of each collected event.
 */
static void event(struct input_handle *handle, struct input_value *value_ptr) {
  /* printk(KERN_INFO "type %d, code %d, value %d", type, code, value); */

  switch (value_ptr->type) {
  case EV_REL: {
    dbg("EV_REL => code %d, value %d", value_ptr->code, value_ptr->value);
    update_mouse_move(value_ptr);
    return;
  }
  case EV_SYN: {
    int x = get_x(MOVEMENT);
    int y = get_y(MOVEMENT);
    if (x || y) {
      dbg("EV_SYN => code %d", value_ptr->code);

      accelerate(&x, &y);
      dbg("accelerated -> (%d, %d)", x, y);
      set_x_move(x);
      set_y_move(y);

      clear_mouse_move();
    }

    return;
  }
  default:
    return;
  }
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0))
#define __cleanup_events 0
#else
#define __cleanup_events 1
#endif

#if __cleanup_events
static unsigned int maccel_events(struct input_handle *handle,
                                  struct input_value *vals,
                                  unsigned int count) {
#else
static void maccel_events(struct input_handle *handle,
                          const struct input_value *vals, unsigned int count) {
#endif
  struct input_value *v;
  for (v = vals; v != vals + count; v++) {
    event(handle, v);
  }

  struct input_value *end = vals;
  for (v = vals; v != vals + count; v++) {
    if (v->type == EV_REL && v->value == NONE_EVENT_VALUE)
      continue;
    if (end != v) {
      *end = *v;
    }
    end++;
  }

  int _count = end - vals;
  handle->dev->num_vals = _count;
#if __cleanup_events
  return _count;
#endif
}

/* Same as Linux's input_register_handle but we always add the handle to the
 * head of handlers */
static int input_register_handle_head(struct input_handle *handle) {
  struct input_handler *handler = handle->handler;
  struct input_dev *dev = handle->dev;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 7))
  /* In 6.11.7 an additional handler pointer was added:
   * https://github.com/torvalds/linux/commit/071b24b54d2d05fbf39ddbb27dee08abd1d713f3
   */
  if (handler->events)
    handle->handle_events = handler->events;
#endif

  int error = mutex_lock_interruptible(&dev->mutex);
  if (error)
    return error;

  list_add_rcu(&handle->d_node, &dev->h_list);
  mutex_unlock(&dev->mutex);
  list_add_tail_rcu(&handle->h_node, &handler->h_list);
  if (handler->start)
    handler->start(handle);
  return 0;
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

  error = input_register_handle_head(handle);
  if (error)
    goto err_free_mem;

  error = input_open_device(handle);
  if (error)
    goto err_unregister_handle;

  printk(KERN_INFO pr_fmt("maccel flags: DEBUG=%s; FIXEDPT_BITS=%d"),
         DEBUG_TEST ? "true" : "false", FIXEDPT_BITS);

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

struct input_handler maccel_handler = {.events = maccel_events,
                                       .connect = maccel_connect,
                                       .disconnect = maccel_disconnect,
                                       .name = "maccel",
                                       .id_table = my_ids};
