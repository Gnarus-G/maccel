#ifndef _HID_PARSER_H
#define _HID_PARSER_H

// HID Descriptors
enum hid_descriptor{
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

#endif //_HID_PARSER_H