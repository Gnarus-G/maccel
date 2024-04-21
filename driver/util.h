#ifndef _UTIL_H
#define _UTIL_H

#define INLINE __attribute__((always_inline)) inline

// HID Descriptors
enum D_hid_descriptor {
  // No data follows after descriptor
  D_END_COLLECTION = 0xC0,

  D_REPORT_ID = 0x84,
  D_INPUT = 0x80,
  D_FEATURE = 0xB0,
  D_REPORT_SIZE = 0x74,
  D_REPORT_COUNT = 0x94,
  D_LOGICAL_MINIMUM = 0x14,
  D_LOGICAL_MAXIMUM = 0x24,
  D_USAGE = 0x08,
  D_USAGE_PAGE = 0x04,
};

// HID data stored after a descriptor
enum hid_data {
  D_USAGE_BUTTON = 0x09,
  D_USAGE_WHEEL = 0x38,
  D_USAGE_X = 0x30,
  D_USAGE_Y = 0x31
};

// Stores the bit offset, bit size, sign and associated report ID of an entry
// for extracting the value from the raw usb_mouse->data buffer
struct report_entry {
  unsigned char id;     // Report ID
  unsigned char offset; // In bits
  unsigned char size;   // In bits
  unsigned char sgn;    // Is this value signed (1) or unsigned (0)?
};

// Stores a collection of important offsets & sizes etc for the received raw
// data in the usb_mouse->data buffer
struct report_positions {
  int report_id_tagged; // When the report descriptor parser recognizes a report
                        // ID is used, this field is set to 1
  struct report_entry button;
  struct report_entry x;
  struct report_entry y;
  struct report_entry wheel;
};

int parse_report_desc(unsigned char *data, int data_len,
                      struct report_positions *data_pos);
int extract_mouse_events(unsigned char *data, int data_len,
                         struct report_positions *data_pos, int *btn, int *x,
                         int *y, int *wheel);

#endif //_UTIL_H
