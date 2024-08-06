#include "./accelk.h"
#include "linux/usb.h"
#include <linux/hid.h>
#include <linux/usb/input.h>

#define TRANSFER_BUFFER_LEN                                                    \
  8 // When this was 16, sometimes clicks would fail to register, 8 is how it is
    // in the linux usbhid driver so it's probably better

typedef struct {
  s8 *data_buf;
  char name[64];
  char phys[128];

  struct input_dev *input_dev;
  struct urb *urb;
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

  input_report_key(dev, BTN_LEFT, data[0] & 0x01);
  input_report_key(dev, BTN_RIGHT, data[0] & 0x02);
  input_report_key(dev, BTN_MIDDLE, data[0] & 0x04);
  input_report_key(dev, BTN_SIDE, data[0] & 0x08);
  input_report_key(dev, BTN_EXTRA, data[0] & 0x10);

  AccelResult result = accelerate(data[1], data[2]);

  input_report_rel(dev, REL_X, result.x);
  input_report_rel(dev, REL_Y, result.y);
  input_report_rel(dev, REL_WHEEL, data[3]);

  input_sync(dev);

resubmit:
  status = usb_submit_urb(u, GFP_ATOMIC);
  if (status) {
    printk(KERN_ERR "couldn't resubmit request block");
  }
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id) {
  int err = -ENOMEM;
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

    err = input_register_device(ctx->input_dev);

    input_set_drvdata(ctx->input_dev, ctx);
  }

  ctx->data_buf =
      usb_alloc_coherent(usb_dev, 8, GFP_ATOMIC, &urb->transfer_dma);
  urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  usb_fill_int_urb(urb, usb_dev, pipe, ctx->data_buf, TRANSFER_BUFFER_LEN,
                   on_complete, ctx, endpoint->bInterval);

  if (!ctx->data_buf || err) {
    goto err_free_urb_transfer_data;
  }

  usb_set_intfdata(intf, ctx);
  printk(KERN_INFO
         "plugged in %s %s <> (%04x:%04x) intf %d; polling interval %d\n",
         ctx->name, ctx->phys, id->idVendor, id->idProduct,
         id->bInterfaceNumber, endpoint->bInterval);

  return 0;

err_free_urb_transfer_data:
  usb_free_coherent(usb_dev, 8, ctx->data_buf, urb->transfer_dma);

err_free_input_dev:
  input_free_device(input_dev);

err_free_ctx_and_urb:
  usb_free_urb(urb);
  kfree(ctx);
  return err;
}

static void disconnect(struct usb_interface *intf) {
  maccel_ctx *ctx = usb_get_intfdata(intf);
  usb_set_intfdata(intf, NULL);
  if (ctx) {
    input_unregister_device(ctx->input_dev);
    usb_free_coherent(interface_to_usbdev(intf), 8, ctx->data_buf,
                      ctx->urb->transfer_dma);
    usb_kill_urb(ctx->urb);
    usb_free_urb(ctx->urb);
    kfree(ctx);
  };

  printk(KERN_INFO "maccel usbmouse removed");
}

static struct usb_device_id maccel_table[] = {
    {USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
                        USB_INTERFACE_PROTOCOL_MOUSE)},
    {} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, maccel_table);

struct usb_driver maccel_usb_driver = {.name = "maccel_usbmouse",
                                       .id_table = maccel_table,
                                       .probe = probe,
                                       .disconnect = disconnect};
