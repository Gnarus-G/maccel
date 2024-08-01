#include "accel.h"
#include "dbg.h"
#include "linux/input-event-codes.h"
#include "linux/input.h"
#include "linux/ktime.h"
#include "linux/mod_devicetable.h"
#include "params.h"
#include <linux/hid.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gnarus-G");
MODULE_DESCRIPTION("Mouse acceleration driver");

#define USB_VENDOR_ID_RAZER 0x1532
#define USB_VENDOR_ID_RAZER_VIPER 0x0078

static bool maccel_match(struct hid_device *dev, bool ignore_special_driver) {
  return dev->type == HID_TYPE_USBMOUSE;
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

static int maccel_raw_event(struct hid_device *hdev, struct hid_report *report,
                            u8 *_data, int size) {

  s8 *data = _data;
  hid_info(hdev, "report id %d", report->id);
  hid_info(hdev, "(x, y) = (%d, %d)", data[1], data[2]);

  switch (report->id) {
  case 0: /* Mouse input */
    AccelResult result = accelerate(data[1], data[2]);
    data[1] = result.x;
    data[2] = result.y;

    hid_info(hdev, "(x, y) = (%d, %d)", data[1], data[2]);

    hid_report_raw_event(hdev, HID_INPUT_REPORT, (u8 *)data, size, 0);
    return 1;

  default: /* unknown report */
    /* Unknown report type; pass upstream */
    hid_info(hdev, "unknown report type %d\n", report->id);
    break;
  }

  return 0;
}

static int maccel_event(struct hid_device *hdev, struct hid_field *field,
                        struct hid_usage *usage, __s32 value) {
  hid_info(hdev, "Event: usage-hid -> %d", usage->hid);
  hid_info(hdev, "Event: usage-code -> %d vs %d", usage->code,
           REL_WHEEL_HI_RES);

  if (usage->code == REL_X) {
    hid_info(hdev, "Event: mouse move x value: %d", *field->new_value);
  }

  if (usage->code == REL_Y) {
    hid_info(hdev, "Event: mouse move y value: %d", *field->new_value);
  }

  hid_info(hdev, "Event: usage-type -> %d", usage->type);
  if (usage->type == EV_REL) {
    hid_info(hdev, "Event: mouse movement");
    return 0;
  }
  /* input_event(field->hidinput->input, usage->type, usage->code, value); */
  /* return 1; */
  return 0;
}

/* static const struct hid_device_id maccel_devices[] = { */
/*     {HID_USB_DEVICE(USB_VENDOR_ID_RAZER, USB_VENDOR_ID_RAZER_VIPER)}, */
/*     {HID_USB_DEVICE(0x258a, 0x0036)}, // Glorious Medel 0 */
/*     {}}; */

static const struct hid_device_id maccel_devices[] = {
    {HID_USB_DEVICE(HID_ANY_ID, HID_ANY_ID)}, {}};

static const struct hid_usage_id maccel_usages[] = {
    {HID_USAGE_ID(HID_ANY_ID, HID_ANY_ID, HID_ANY_ID)},
    /* {HID_USAGE_ID(HID_GD_Y, EV_REL, REL_Y)}, */
    /* {HID_USAGE_ID(HID_ANY_ID, EV_REL, REL_Y)}, */
    {HID_TERMINATOR, HID_TERMINATOR, HID_TERMINATOR}};

MODULE_DEVICE_TABLE(hid, maccel_devices);

static struct hid_driver maccel_driver = {
    .name = "maccel",
    .id_table = maccel_devices,
    .raw_event = maccel_raw_event,
    .usage_table = maccel_usages,
    .event = maccel_event,
    /* .match = maccel_match, */
};

module_hid_driver(maccel_driver);
