// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) 1999-2001 Vojtech Pavlik
 *
 *  USB HIDBP Mouse support
 */

/*
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

// Leetmouse Mod BEGIN
#include "accel.h"
#include "config.h"
#include "util.h"
// Leetmouse Mod END

#include <linux/hid.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb/input.h>

/* for apple IDs */
/*                                                              //Leetmouse Mod
BEGIN #ifdef CONFIG_USB_HID_MODULE #include "../hid-ids.h" #endif
*/                                                              //Leetmouse Mod END

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.6"
#define DRIVER_AUTHOR "Vojtech Pavlik <vojtech@ucw.cz>"
#define DRIVER_DESC                                                            \
  "USB HID mouse driver with acceleration (LEETMOUSE)" // Leetmouse Mod

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

struct usb_mouse {
  char name[128];
  char phys[64];
  struct usb_device *usbdev;
  struct input_dev *dev;
  struct urb *irq;

  signed char *data;
  dma_addr_t data_dma;

  struct report_positions *data_pos;
};

static void usb_mouse_irq(struct urb *urb) {
  struct usb_mouse *mouse = urb->context;
  signed char *data = mouse->data;
  struct input_dev *dev = mouse->dev;
  signed int btn, x, y, wheel; // Leetmouse Mod
  int status;

  switch (urb->status) {
  case 0: /* success */
    break;
  case -ECONNRESET: /* unlink */
  case -ENOENT:
  case -ESHUTDOWN:
    return;
    // Leetmouse Mod BEGIN
  case -EOVERFLOW:
    printk("LEETMOUSE: EOVERFLOW. Try to increase BUFFER_SIZE from %d to %d in "
           "'config.h'",
           BUFFER_SIZE, 2 * BUFFER_SIZE);
    goto resubmit;
    // Leetmouse Mod END
  /* -EPIPE:  should clear the halt */
  default: /* error */
    goto resubmit;
  }

  // Leetmouse Mod BEGIN
  if (!extract_mouse_events(data, BUFFER_SIZE, mouse->data_pos, &btn, &x, &y,
                            &wheel)) {
    input_report_key(dev, BTN_LEFT, btn & 0x01);
    input_report_key(dev, BTN_RIGHT, btn & 0x02);
    input_report_key(dev, BTN_MIDDLE, btn & 0x04);
    input_report_key(dev, BTN_SIDE, btn & 0x08);
    input_report_key(dev, BTN_EXTRA, btn & 0x10);
    if (!accelerate(&x, &y, &wheel)) {
      input_report_rel(dev, REL_X, x);
      input_report_rel(dev, REL_Y, y);
      input_report_rel(dev, REL_WHEEL, wheel);
    }
  }
  // Leetmouse Mod END

  input_sync(dev);
resubmit:
  status = usb_submit_urb(urb, GFP_ATOMIC);
  if (status)
    dev_err(&mouse->usbdev->dev,
            "can't resubmit intr, %s-%s/input0, status %d\n",
            mouse->usbdev->bus->bus_name, mouse->usbdev->devpath, status);
}

static int usb_mouse_open(struct input_dev *dev) {
  struct usb_mouse *mouse = input_get_drvdata(dev);

  mouse->irq->dev = mouse->usbdev;
  if (usb_submit_urb(mouse->irq, GFP_KERNEL))
    return -EIO;

  return 0;
}

static void usb_mouse_close(struct input_dev *dev) {
  struct usb_mouse *mouse = input_get_drvdata(dev);

  usb_kill_urb(mouse->irq);
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

static int usb_mouse_probe(struct usb_interface *intf,
                           const struct usb_device_id *id) {
  struct usb_device *dev = interface_to_usbdev(intf);
  struct usb_host_interface *interface;
  struct usb_endpoint_descriptor *endpoint;
  struct usb_mouse *mouse;
  struct input_dev *input_dev;
  int pipe, maxp;
  int ret = -ENOMEM;
  // Leetmouse Mod BEGIN
  // Taken from drivers/hid/usbhid/hid-core.c (usbhid_probe, usbhid_parse,
  // usbhid_start)
  // ##########################################################################
  // This code extracts the report descriptor from the mouse. Might not be safe
  // though (see
  // https://github.com/torvalds/linux/blob/2a1d7946fa53cea2083e5981ff55a8176ab2be6b/drivers/hid/usbhid/hid-core.c#L1001)
  // I know, this is a total hack! The cleanest way would be to write a real HID
  // driver, with the HID subsystem taking care for probing the device.
  // This is (probably) planned. Due to my limited knowledge about the HID
  // subsystem, I have chosen to go this hacky route of merging usbmouse.c and
  // hid-core.c code for now. Expect this to (probably) change in the future!
  // ##########################################################################
  struct hid_descriptor *hdesc;
  struct report_positions *rpos;
  unsigned int rsize = 0;
  int num_descriptors;
  char *rdesc;
  unsigned int n = 0;
  size_t offset = offsetof(struct hid_descriptor, desc);

  // Leetmouse Mod END
  interface = intf->cur_altsetting;

  if (interface->desc.bNumEndpoints != 1)
    return -ENODEV;

  endpoint = &interface->endpoint[0].desc;
  if (!usb_endpoint_is_int_in(endpoint))
    return -ENODEV;

  pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
  maxp = usb_maxpacket(dev, pipe);

  mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
  input_dev = input_allocate_device();
  if (!mouse || !input_dev)
    goto fail1;

  mouse->data = usb_alloc_coherent(dev, BUFFER_SIZE, GFP_ATOMIC,
                                   &mouse->data_dma); // Leetmouse Mod
  if (!mouse->data)
    goto fail1;

  // Leetmouse Mod BEGIN
  if (usb_get_extra_descriptor(interface, HID_DT_HID, &hdesc) &&
      (!interface->desc.bNumEndpoints ||
       usb_get_extra_descriptor(&interface->endpoint[0], HID_DT_HID, &hdesc))) {
    dbg_hid("class descriptor not present\n");
    ret = -ENODEV;
    goto fail1;
  }

  if (hdesc->bLength < sizeof(struct hid_descriptor)) {
    dbg_hid("hid descriptor is too short\n");
    ret = -EINVAL;
    goto fail1;
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
    goto fail1;
  }

  rdesc = kmalloc(rsize, GFP_KERNEL);
  if (!rdesc)
    goto fail1;

  // hid_set_idle(dev, interface->desc.bInterfaceNumber, 0, 0);

  ret = hid_get_class_descriptor(dev, interface->desc.bInterfaceNumber,
                                 HID_DT_REPORT, rdesc, rsize);
  if (ret < 0) {
    dbg_hid("reading report descriptor failed\n");
    kfree(rdesc);
    goto fail1;
  }

  rpos = kmalloc(sizeof(struct report_positions), GFP_KERNEL);
  if (!rpos) {
    kfree(rdesc);
    goto fail1;
  }
  mouse->data_pos = rpos;

  // Parse the descriptor and delete it
  ret = parse_report_desc(rdesc, rsize, rpos);
  kfree(rdesc);
  if (ret < 0)
    goto fail1_5;
  // Leetmouse Mod END

  mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
  if (!mouse->irq)
    goto fail2;

  mouse->usbdev = dev;
  mouse->dev = input_dev;

  if (dev->manufacturer)
    strscpy(mouse->name, dev->manufacturer, sizeof(mouse->name));

  if (dev->product) {
    if (dev->manufacturer)
      strlcat(mouse->name, " ", sizeof(mouse->name));
    strlcat(mouse->name, dev->product, sizeof(mouse->name));
  }

  if (!strlen(mouse->name))
    snprintf(mouse->name, sizeof(mouse->name), "USB HIDBP Mouse %04x:%04x",
             le16_to_cpu(dev->descriptor.idVendor),
             le16_to_cpu(dev->descriptor.idProduct));

  usb_make_path(dev, mouse->phys, sizeof(mouse->phys));
  strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

  input_dev->name = mouse->name;
  input_dev->phys = mouse->phys;
  usb_to_input_id(dev, &input_dev->id);
  input_dev->dev.parent = &intf->dev;

  input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
  input_dev->keybit[BIT_WORD(BTN_MOUSE)] =
      BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
  input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
  input_dev->keybit[BIT_WORD(BTN_MOUSE)] |=
      BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
  input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

  input_set_drvdata(input_dev, mouse);

  input_dev->open = usb_mouse_open;
  input_dev->close = usb_mouse_close;

  usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
                   (maxp > BUFFER_SIZE ? BUFFER_SIZE : maxp), // Leetmouse Mod
                   usb_mouse_irq, mouse, endpoint->bInterval);
  mouse->irq->transfer_dma = mouse->data_dma;
  mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  ret = input_register_device(mouse->dev); // Leetmouse Mod
  if (ret)                                 // Leetmouse Mod
    goto fail3;

  usb_set_intfdata(intf, mouse);
  return 0;

fail3:
  usb_free_urb(mouse->irq);
fail2:
  usb_free_coherent(dev, BUFFER_SIZE, mouse->data,
                    mouse->data_dma); // Leetmouse Mod
fail1_5:
  kfree(mouse->data_pos);
fail1:
  input_free_device(input_dev);
  kfree(mouse);
  return ret; // Leetmouse Mod
}

static void usb_mouse_disconnect(struct usb_interface *intf) {
  struct usb_mouse *mouse = usb_get_intfdata(intf);

  usb_set_intfdata(intf, NULL);
  if (mouse) {
    usb_kill_urb(mouse->irq);
    input_unregister_device(mouse->dev);
    usb_free_urb(mouse->irq);
    // Leetmouse Mod BEGIN
    usb_free_coherent(interface_to_usbdev(intf), BUFFER_SIZE, mouse->data,
                      mouse->data_dma);
    kfree(mouse->data_pos);
    // Leetmouse Mod END
    kfree(mouse);
  }
}

static const struct usb_device_id usb_mouse_id_table[] = {
    {USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
                        USB_INTERFACE_PROTOCOL_MOUSE)},
    {} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
    .name = "leetmouse", // Leetmouse Mod
    .probe = usb_mouse_probe,
    .disconnect = usb_mouse_disconnect,
    .id_table = usb_mouse_id_table,
};

module_usb_driver(usb_mouse_driver);
