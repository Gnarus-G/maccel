#include "./input_handler.h"
#include "input_echo.h"

/*
 * We initialize the character driver for the userspace visualizations,
 * and we register the input_handler.
 */
static int __init driver_initialization(void) {
  int error;
  error = create_char_device();
  if (error)
    return error;

  error = input_register_handler(&maccel_handler);
  if (error)
    goto err_free_chrdev;

  return 0;

err_free_chrdev:
  destroy_char_device();
  return error;
}

static void __exit driver_exit(void) {
  input_unregister_handler(&maccel_handler);
  destroy_char_device();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver.");

module_init(driver_initialization);
module_exit(driver_exit);
