#include "accel.h"
#include "linux/ktime.h"
#include "params.h"
#include "util.h"
#include <linux/hid.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

#define TRANSFER_BUFFER_LEN                                                    \
  8 // When this was 16, sometimes clicks would fail to register, 8 is how it
    // is
    // in the linux usbhid driver so it's probably better

typedef struct {
  s8 *data_buf;
  char name[64];
  char phys[128];

  struct input_dev *input_dev;
  struct urb *urb;

  struct report_positions *data_pos;
} maccel_ctx;

static int usb_mouse_open(struct input_dev *dev) {
  maccel_ctx *ctx = input_get_drvdata(dev);

  if (usb_submit_urb(ctx->urb, GFP_KERNEL)) {
    return -EIO;
  }

  return 0;
}

static void usb_mouse_close(struct input_dev *dev) {
  maccel_ctx *ctx = input_get_drvdata(dev);

  usb_kill_urb(ctx->urb);
}

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

static void on_complete(struct urb *u) {
  s32 status = 0;
  maccel_ctx *ctx = u->context;
  struct input_dev *dev = ctx->input_dev;
  s8 *data = ctx->data_buf;

  dbg("name: %s", ctx->name);
  dbg("phys: %s", ctx->phys);

  switch (u->status) {
  case 0:
    break;
  case -ECONNRESET:
  case -ENOENT:
  case -ESHUTDOWN:
    return;
  case -EOVERFLOW:
    printk(KERN_ALERT "EOVERFLOW");
  default:
    printk(KERN_ALERT "unknown status; will resubmit request block");
    goto resubmit;
  }

  signed int btn, x, y, wheel; // Leetmouse Mod

  if (!extract_mouse_events(data, TRANSFER_BUFFER_LEN, ctx->data_pos, &btn, &x,
                            &y, &wheel)) {
    input_report_key(dev, BTN_LEFT, btn & 0x01);
    input_report_key(dev, BTN_RIGHT, btn & 0x02);
    input_report_key(dev, BTN_MIDDLE, btn & 0x04);
    input_report_key(dev, BTN_SIDE, btn & 0x08);
    input_report_key(dev, BTN_EXTRA, btn & 0x10);

    AccelResult result = accelerate(x, y);

    input_report_rel(dev, REL_X, result.x);
    input_report_rel(dev, REL_Y, result.y);
    input_report_rel(dev, REL_WHEEL, wheel);
  }

  input_sync(dev);

resubmit:
  status = usb_submit_urb(u, GFP_ATOMIC);
  if (status) {
    printk(KERN_ERR "couldn't resubmit request block");
  }
}

static int hid_get_class_descriptor(struct usb_device *dev, int ifnum,
                                    unsigned char type, void *buf, int size) {
  int result, retries = 4;

  memset(buf, 0, size);

  do {
    result =
        usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_GET_DESCRIPTOR,
                        USB_RECIP_INTERFACE | USB_DIR_IN, (type << 8), ifnum,
                        buf, size, USB_CTRL_GET_TIMEOUT);
    retries--;
  } while (result < size && retries);
  return result;
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id) {
  int ret = -ENOMEM;
  struct usb_device *usb_dev = interface_to_usbdev(intf);
  struct urb *urb = usb_alloc_urb(0, GFP_KERNEL);
  struct usb_endpoint_descriptor *endpoint =
      &intf->cur_altsetting->endpoint[0].desc;
  u32 pipe = usb_rcvintpipe(usb_dev, endpoint->bEndpointAddress);
  maccel_ctx *ctx = kzalloc(sizeof(maccel_ctx), GFP_KERNEL);

  if (!usb_endpoint_is_int_in(endpoint)) {
    return -ENODEV;
  }

  if (!urb || !ctx) {
    goto err_free_ctx_and_urb;
  }

  urb->dev = usb_dev;
  ctx->urb = urb;

  struct input_dev *input_dev = input_allocate_device();
  { // This section where we marry the usb interface to an input device
    // is ripped from linux source code
    // https://github.com/torvalds/linux/blob/master/drivers/hid/usbhid/usbmouse.c

    if (!input_dev) {
      goto err_free_input_dev;
    }

    if (usb_dev->manufacturer) {
      strscpy(ctx->name, usb_dev->manufacturer, sizeof(ctx->name));
    }

    if (usb_dev->product) {
      if (usb_dev->manufacturer) {
        strlcat(ctx->name, " ", sizeof(ctx->name));
      }
      strlcat(ctx->name, usb_dev->product, sizeof(ctx->name));
    }

    if (!strlen(ctx->name))
      snprintf(ctx->name, sizeof(ctx->name), "USB HIDBP Mouse %04x:%04x",
               le16_to_cpu(usb_dev->descriptor.idVendor),
               le16_to_cpu(usb_dev->descriptor.idProduct));

    usb_make_path(usb_dev, ctx->phys, sizeof(ctx->phys));
    strlcat(ctx->phys, "/input0", sizeof(ctx->phys));

    usb_to_input_id(usb_dev, &input_dev->id);
    input_dev->name = ctx->name;
    input_dev->phys = ctx->phys;
    input_dev->dev.parent = &intf->dev;
    input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
    input_dev->keybit[BIT_WORD(BTN_MOUSE)] =
        BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
    input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
    input_dev->keybit[BIT_WORD(BTN_MOUSE)] |=
        BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
    input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);
    input_dev->open = usb_mouse_open;
    input_dev->close = usb_mouse_close;

    ctx->input_dev = input_dev;

    ret = input_register_device(ctx->input_dev);

    input_set_drvdata(ctx->input_dev, ctx);
  }

  {
    // Leetmouse Mod Begin: Hid report descriptor parsing
    struct hid_descriptor *hdesc;
    struct report_positions *rpos;
    unsigned int rsize = 0;
    int num_descriptors;
    char *rdesc;
    unsigned int n = 0;
    size_t offset = offsetof(struct hid_descriptor, desc);
    struct usb_host_interface *interface;

    interface = intf->cur_altsetting;

    if (usb_get_extra_descriptor(interface, HID_DT_HID, &hdesc) &&
        (!interface->desc.bNumEndpoints ||
         usb_get_extra_descriptor(&interface->endpoint[0], HID_DT_HID,
                                  &hdesc))) {
      dbg_hid("class descriptor not present\n");
      ret = -ENODEV;
      goto err_free_input_dev;
    }

    if (hdesc->bLength < sizeof(struct hid_descriptor)) {
      dbg_hid("hid descriptor is too short\n");
      ret = -EINVAL;
      goto err_free_input_dev;
    }

    num_descriptors =
        min_t(int, hdesc->bNumDescriptors,
              (hdesc->bLength - offset) / sizeof(struct hid_class_descriptor));

    for (n = 0; n < num_descriptors; n++)
      if (hdesc->desc[n].bDescriptorType == HID_DT_REPORT)
        rsize = le16_to_cpu(hdesc->desc[n].wDescriptorLength);

    if (!rsize || rsize > HID_MAX_DESCRIPTOR_SIZE) {
      dbg_hid("weird size of report descriptor (%u)\n", rsize);
      ret = -EINVAL;
      goto err_free_input_dev;
    }

    rdesc = kmalloc(rsize, GFP_KERNEL);
    if (!rdesc)
      goto err_free_input_dev;

    ret = hid_get_class_descriptor(usb_dev, interface->desc.bInterfaceNumber,
                                   HID_DT_REPORT, rdesc, rsize);
    if (ret < 0) {
      dbg_hid("reading report descriptor failed\n");
      kfree(rdesc);
      goto err_free_input_dev;
    }

    rpos = kmalloc(sizeof(struct report_positions), GFP_KERNEL);
    if (!rpos) {
      kfree(rdesc);
      goto err_free_input_dev;
    }
    ctx->data_pos = rpos;

    // Parse the descriptor and delete it
    ret = parse_report_desc(rdesc, rsize, rpos);
    kfree(rdesc);
    if (ret < 0)
      goto err_free_report_desc;
    // Leetmouse Mod END
  }

  int maxp = usb_maxpacket(usb_dev, pipe);

  ctx->data_buf = usb_alloc_coherent(usb_dev, TRANSFER_BUFFER_LEN, GFP_ATOMIC,
                                     &urb->transfer_dma);
  urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  usb_fill_int_urb(urb, usb_dev, pipe, ctx->data_buf,
                   (maxp > TRANSFER_BUFFER_LEN ? TRANSFER_BUFFER_LEN : maxp),
                   on_complete, ctx, endpoint->bInterval);

  if (!ctx->data_buf || ret) {
    goto err_free_urb_transfer_data;
  }

  usb_set_intfdata(intf, ctx);
  printk(KERN_INFO
         "plugged in %s %s <> (%04x:%04x) intf %d; polling interval %d\n",
         ctx->name, ctx->phys, id->idVendor, id->idProduct,
         id->bInterfaceNumber, endpoint->bInterval);

  return 0;

err_free_urb_transfer_data:
  usb_free_coherent(usb_dev, TRANSFER_BUFFER_LEN, ctx->data_buf,
                    urb->transfer_dma);

err_free_report_desc:
  kfree(ctx->data_pos);

err_free_input_dev:
  input_free_device(input_dev);

err_free_ctx_and_urb:
  usb_free_urb(urb);
  kfree(ctx);
  return ret;
}

static void disconnect(struct usb_interface *intf) {
  maccel_ctx *ctx = usb_get_intfdata(intf);
  usb_set_intfdata(intf, NULL);
  if (ctx) {
    input_unregister_device(ctx->input_dev);
    usb_free_coherent(interface_to_usbdev(intf), TRANSFER_BUFFER_LEN,
                      ctx->data_buf, ctx->urb->transfer_dma);
    usb_kill_urb(ctx->urb);
    usb_free_urb(ctx->urb);
    kfree(ctx);
  };

  printk(KERN_INFO "maccel removed");
}

static struct usb_device_id maccel_table[] = {
    {USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
                        USB_INTERFACE_PROTOCOL_MOUSE)},
    {} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, maccel_table);

static struct usb_driver maccel_driver = {.name = "maccel",
                                          .id_table = maccel_table,
                                          .probe = probe,
                                          .disconnect = disconnect};

module_usb_driver(maccel_driver);
