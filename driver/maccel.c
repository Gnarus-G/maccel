#include "./input_handler.h"
#include "./usbmouse.h"
#include "input_echo.h"

/*
 * We initialize the usb driver for the usb_mouse driver, and the character
 * driver for the userspace visualizations.
 *
 * Some may prefer to bind to this usbmouse driver if their mouse allows.
 */
static int __init my_init(void) {
  int error;
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
  return error;
}

static void __exit my_exit(void) {
  input_unregister_handler(&maccel_handler);

  destroy_char_device();

  usb_deregister(&maccel_usb_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

module_init(my_init);
module_exit(my_exit);
