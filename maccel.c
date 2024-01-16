#include "linux/init.h"
#include "linux/kern_levels.h"
#include "linux/mod_devicetable.h"
#include "linux/printk.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

static struct usb_device_id maccel_table[] = {{USB_DEVICE(0x1532, 0x0078)}, {}};

MODULE_DEVICE_TABLE(usb, maccel_table);

int probe(struct usb_interface *intf, const struct usb_device_id *id) {
  printk(KERN_INFO "plugged in (%04x:%04x)\n", id->idVendor, id->idProduct);
  return 0;
}

void disconnect(struct usb_interface *intf) {
  printk(KERN_INFO "maccel removed");
}

static struct usb_driver maccel_driver = {.name = "maccel",
                                          .id_table = maccel_table,
                                          .probe = probe,
                                          .disconnect = disconnect};

static int __init maccel_init(void) {
  int ret = -1;
  printk(KERN_INFO "registering driver");
  ret = usb_register(&maccel_driver);
  printk(KERN_INFO "registration complete");
  return ret;
}

static void __exit maccel_exit(void) {
  usb_deregister(&maccel_driver);
  printk(KERN_INFO "unregistration complete");
}

module_init(maccel_init);
module_exit(maccel_exit);
