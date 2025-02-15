#include "./input_handler.h"
#include "./usbmouse.h"
#include "input_echo.h"

// The virtual_input_dev is declared for/in the maccel_input_handler module.
// This initializes it.
static int create_virtual_device(void) {
  int error;

  VIRTUAL_INPUT_DEV = input_allocate_device();
  if (!VIRTUAL_INPUT_DEV) {
    printk(KERN_ERR "Failed to allocate virtual input device\n");
    return -ENOMEM;
  }

  VIRTUAL_INPUT_DEV->name = "maccel [Virtual Mouse]";
  VIRTUAL_INPUT_DEV->id.bustype = BUS_USB;
  /* virtual_input_dev->id.vendor = 0x1234; */
  /* virtual_input_dev->id.product = 0x5678; */
  VIRTUAL_INPUT_DEV->id.version = 1;

  // Set the supported event types and codes for the virtual device
  // and for some reason not setting some EV_KEY bits causes a noticeable
  // difference in the values we operate on, leading to a different
  // acceleration behavior than we expect.
  set_bit(EV_KEY, VIRTUAL_INPUT_DEV->evbit);
  set_bit(BTN_LEFT, VIRTUAL_INPUT_DEV->keybit);

  set_bit(EV_REL, VIRTUAL_INPUT_DEV->evbit);
  for (u32 code = REL_X; code < REL_CNT; code++) {
    set_bit(code, VIRTUAL_INPUT_DEV->relbit);
  }

  error = input_register_device(VIRTUAL_INPUT_DEV);
  if (error) {
    printk(KERN_ERR "Failed to register virtual input device\n");
    input_free_device(VIRTUAL_INPUT_DEV);
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
  error = usb_register(&maccel_usb_driver);
  if (error)
    goto err_free_vdev;

  error = create_char_device();
  if (error)
    goto err_unregister_usb;

  error = input_register_handler(&maccel_handler);
  if (error)
    goto err_free_chrdev;

  return 0;

err_free_chrdev:
  destroy_char_device();

err_unregister_usb:
  usb_deregister(&maccel_usb_driver);

err_free_vdev:
  input_unregister_device(VIRTUAL_INPUT_DEV);
  input_free_device(VIRTUAL_INPUT_DEV);
  return error;
}

static void __exit my_exit(void) {
  input_unregister_handler(&maccel_handler);

  destroy_char_device();

  usb_deregister(&maccel_usb_driver);

  input_unregister_device(VIRTUAL_INPUT_DEV);
  input_free_device(VIRTUAL_INPUT_DEV);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

module_init(my_init);
module_exit(my_exit);
