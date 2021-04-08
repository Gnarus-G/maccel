#ifndef _UTIL_H
#define _UTIL_H

// HID Descriptors
enum D_hid_descriptor{
    // No data follows after descriptor
    D_END_COLLECTION = 0xC0,

    // Single-Byte data follows after descriptor
    D_INPUT = 0x81,
    D_FEATURE = 0xB1,
    D_REPORT_SIZE = 0x75,
    D_REPORT_COUNT = 0x95,
    D_LOGICAL_MINIMUM = 0x15,
    D_LOGICAL_MAXIMUM = 0x25,
    D_USAGE = 0x09,
    D_USAGE_PAGE = 0x05,

    // Multi-Byte (2) data follows after descriptors
    D_LOGICAL_MINIMUM_MB = 0x16,
    D_LOGICAL_MAXIMUM_MB = 0x26,
    D_USAGE_MB = 0x0A,
    D_USAGE_PAGE_MB = 0x06
};

// HID data stored after a descriptor
enum hid_data{
    D_USAGE_BUTTON = 0x09,
    D_USAGE_WHEEL = 0x38,
    D_USAGE_X = 0x30,
    D_USAGE_Y = 0x31
};

//Stores the bit offset and bit size in the raw reported data structure of usb_mouse::data
struct report_entry {
	unsigned char offset;	// In bits
	unsigned char size;		// In bits
};

//Stores a collection of important offsets & sizes for report data in usb_mouse::data
struct report_positions {
	struct report_entry button;
	struct report_entry x;
	struct report_entry y;
	struct report_entry wheel;
};

void atof(const char *str, int len, float *result);
int Leet_round(float x);
void Q_sqrt(float *number);
int parse_report_desc(unsigned char *data, int data_len, struct report_positions *data_pos);
int extract_mouse_events(unsigned char *data, int data_len, struct report_positions *data_pos, int *btn, int *x, int *y, int *wheel);

#endif  //_UTIL_H
