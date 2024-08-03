#include "accel.h"
#include "linux/bits.h"
#include "linux/fs.h"
#include "linux/gfp_types.h"
#include "linux/input-event-codes.h"
#include "linux/kern_levels.h"
#include "linux/ktime.h"
#include "linux/mod_devicetable.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/spinlock.h"
#include "linux/workqueue.h"
#include "params.h"
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver");

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

struct ctx {
  spinlock_t lock;
  struct workqueue_struct *wq;
  unsigned long last_event_jiffies;
};

struct maccel_data {
  struct work_struct work;
  struct input_handle *handle;
  unsigned int type;
  unsigned int code;
  int value;
};

static void maccel_work(struct work_struct *work) {
  struct maccel_data *data = container_of(work, struct maccel_data, work);
  static int x, y = 0;
  struct input_handle *handle = data->handle;
  unsigned int type = data->type;
  unsigned int code = data->code;
  int value = data->value;

  if (type != EV_REL) {
    input_inject_event(handle, type, code, value);
    kfree(data);
    return;
  }

  printk(KERN_INFO "Event Before: type %d, code %d, value %d\n", type, code,
         value);

  AccelResult acceled;

  if (code == REL_X) {
    x = value;
    acceled = accelerate(x, y);
    input_inject_event(handle, EV_REL, code, acceled.x);
  }

  if (code == REL_Y) {
    y = value;
    acceled = accelerate(x, y);
    input_inject_event(handle, EV_REL, code, acceled.y);
  }
  printk(KERN_INFO "accel: (%d, %d) -> (%d, %d)", x, y, acceled.x, acceled.y);
  input_sync(handle->dev);

  kfree(data);
}

static bool maccel_filter(struct input_handle *handle, unsigned int type,

                          unsigned int code, int value) {
  struct ctx *ctx = handle->private;

  unsigned long flags;
  spin_lock_irqsave(&ctx->lock, flags);

  unsigned long curr_jiffies = jiffies;
  /* printk(KERN_INFO "jiffies, last %lu vs now %lu", ctx->last_event_jiffies,
   */
  /*        curr_jiffies); */
  if (time_after(curr_jiffies, ctx->last_event_jiffies)) {
    ctx->last_event_jiffies = curr_jiffies;

    struct maccel_data *data = kmalloc(sizeof(*data), GFP_ATOMIC);
    if (!data) {
      printk(KERN_ERR "Failed to allocate work data\n");
      return false;
    }

    INIT_WORK(&data->work, maccel_work);
    data->handle = handle;
    data->type = type;
    data->code = code;
    data->value = value;
    queue_work(ctx->wq, &data->work);

    spin_unlock_irqrestore(&ctx->lock, flags);
    return true;
  }

  spin_unlock_irqrestore(&ctx->lock, flags);
  return false;
}

static bool maccel_match(struct input_handler *handler, struct input_dev *dev) {
  struct hid_device *hdev = to_hid_device(dev->dev.parent);

  printk(KERN_INFO "maccel handler found a pointer device named: %s",
         hdev->name);
  printk(KERN_INFO "maccel handler found a mouse? %s",
         hdev->type == HID_TYPE_USBMOUSE ? "true" : "false");

  return hdev->type == HID_TYPE_USBMOUSE;
}

static int maccel_connect(struct input_handler *handler, struct input_dev *dev,
                          const struct input_device_id *id) {
  struct input_handle *handle;
  struct ctx *ctx;
  int error;

  handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!handle)
    return -ENOMEM;

  ctx = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
  if (!ctx) {
    kfree(handle);
    return -ENOMEM;
  }

  handle->dev = input_get_device(dev);
  handle->handler = handler;
  handle->name = "maccel";
  handle->private = ctx;
  ctx->last_event_jiffies = 0;
  spin_lock_init(&ctx->lock);

  ctx->wq = create_singlethread_workqueue("maccel_event_queue");

  if (!ctx->wq) {
    error = -ENOMEM;
    goto err_free_mem;
  }

  error = input_register_handle(handle);
  if (error)
    goto err_destroy_wq;

  error = input_open_device(handle);
  if (error)
    goto err_unregister_handle;

  printk(KERN_INFO pr_fmt("maccel handler connecting device: %s (%s at %s)"),
         dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");

  return 0;

err_unregister_handle:
  input_unregister_handle(handle);

err_destroy_wq:
  destroy_workqueue(ctx->wq);

err_free_mem:
  kfree(ctx);
  kfree(handle);
  return error;
}

static void maccel_disconnect(struct input_handle *handle) {
  struct ctx *ctx = handle->private;
  flush_workqueue(ctx->wq);
  destroy_workqueue(ctx->wq);

  input_close_device(handle);
  input_unregister_handle(handle);
  kfree(ctx);
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

static int __init my_init(void) {
  return input_register_handler(&maccel_handler);
}

static void __exit my_exit(void) { input_unregister_handler(&maccel_handler); }

module_init(my_init);
module_exit(my_exit);
