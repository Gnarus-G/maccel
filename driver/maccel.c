#include "linux/init.h"
#include "linux/module.h"
#include "linux/usb.h"

#include "./input_handler.h"
#include "./usbmouse.h"

// The virtual_input_dev is declared for/in the maccel_input_handler module.
// This initializes it.
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

/*
 * We initialize the virtual_input_dev for input_handler
 * and the usb driver for the usb_mouse driver -
 * Some may prefer to bind to this usbmouse driver if their mouse allows.
 */
static int __init my_init(void) {
  int error;
  error = create_virtual_device();
  if (error) {
    return error;
  }
  error = usb_register_driver(&maccel_usb_driver, THIS_MODULE, KBUILD_MODNAME);
  if (error) {
    input_unregister_device(virtual_input_dev);
    input_free_device(virtual_input_dev);
    return error;
  }

  return input_register_handler(&maccel_handler);
}

static void __exit my_exit(void) {
  input_unregister_handler(&maccel_handler);
  usb_deregister(&maccel_usb_driver);
  input_unregister_device(virtual_input_dev);
  input_free_device(virtual_input_dev);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

module_init(my_init);
module_exit(my_exit);
