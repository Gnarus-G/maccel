#include <iostream>
using namespace std;

//#include "linux/string.h" //Necessary for memcpy to work inside kernel module
#include <cstring>           //memcpy
#include "hid_parser.h"

// From the steelseries mouse
unsigned char desc[] =  {
    //Steelseries Rival 600
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0xA1, 0x02, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x08, 0x15, 0x00, 0x25, 0x01, 0x95, 0x08, 0x75, 0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x30,
    0x09, 0x31, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06, 0x09, 0x38,
    0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0xC0, 0xA1, 0x02, 0x05, 0x0C, 0x0A,
    0x38, 0x02, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0xC0, 0xA1, 0x02, 0x06,
    0xC1, 0xFF, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x09, 0xF0, 0x95, 0x02, 0x81, 0x02, 0xC0,
    0xC0, 0xC0
    
    /*
    // Logitech G5
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x08,
    0x15, 0x00, 0x25, 0x01, 0x95, 0x08, 0x75, 0x01, 0x81, 0x02, 0x06, 0x00, 0xFF, 0x09, 0x40, 0x15,
    0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x05, 0x01, 0x09, 0x38, 0x95, 0x01, 0x81,
    0x06, 0x05, 0x0C, 0x0A, 0x38, 0x02, 0x95, 0x01, 0x81, 0x06, 0x05, 0x01, 0x16, 0x01, 0x80, 0x26,
    0xFF, 0x7F, 0x75, 0x10, 0x95, 0x02, 0x09, 0x30, 0x09, 0x31, 0x81, 0x06, 0x05, 0x09, 0x19, 0x09,
    0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x95, 0x08, 0x75, 0x01, 0x81, 0x02, 0xC0, 0xC0
    */

    /*
    //Coolermaster mm710
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x10,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10, 0x81, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
    0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06, 0x09, 0x38, 0x15, 0x81,
    0x25, 0x7F, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0x05, 0x0C, 0x0A, 0x38, 0x02, 0x95, 0x01, 0x81,
    0x06, 0xC0, 0xC0
    */

    /*
    //Swiftpoint tracer
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x95, 0x10, 0x75, 0x01,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x81, 0x02, 0x75, 0x10, 0x95, 0x02,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, 0x81, 0x06, 0xA1, 0x02,
    0x85, 0x02, 0x09, 0x48, 0x95, 0x01, 0x75, 0x02, 0x15, 0x00, 0x25, 0x01, 0x35, 0x01, 0x45, 0x78,
    0xB1, 0x02, 0x85, 0x01, 0x75, 0x10, 0x95, 0x01, 0x09, 0x38, 0x35, 0x00, 0x45, 0x00, 0x16, 0x01,
    0x80, 0x26, 0xFF, 0x7F, 0x81, 0x06, 0xC0, 0xA1, 0x02, 0x85, 0x02, 0x09, 0x48, 0x95, 0x01, 0x75,
    0x02, 0x15, 0x00, 0x25, 0x01, 0x35, 0x01, 0x45, 0x78, 0xB1, 0x02, 0x35, 0x00, 0x45, 0x00, 0x75,
    0x04, 0xB1, 0x03, 0x85, 0x01, 0x75, 0x10, 0x95, 0x01, 0x05, 0x0C, 0x0A, 0x38, 0x02, 0x16, 0x01,
    0x80, 0x26, 0xFF, 0x7F, 0x81, 0x06, 0xC0, 0xC0, 0xC0
    */
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

//Length of "ctl word + data"
inline int c_len(const unsigned char c){
    switch(c){
        case D_END_COLLECTION: return 1;
        case D_USAGE_MB: return 3;
        case D_USAGE_PAGE_MB: return 3;
        case D_LOGICAL_MAXIMUM_MB: return 3;
        case D_LOGICAL_MINIMUM_MB: return 3;
    }
    
    return 2;
}

//This is the most crudest HID descriptor parser EVER.
//We will skip most control words until we found an interesting one
//We also assume, that the first button-definition we will find is the most important one,
//so we will ignore any further button definitions
int parse_report_desc(unsigned char *data, int data_len, struct report_positions *data_pos)
{
    int r_count = 0, r_size = 0, r_usage_a = 0, r_usage_b = 0;
    unsigned char c, d, button = 0;

    unsigned int offset = 0;    //Offset in bits

    unsigned int i = 0;
    while(i < data_len){
        c = data[i];                // Control word
        if(i < data_len) d = data[i+1];  // Data after control word

        //Determine the size
        if(c == D_REPORT_SIZE)  r_size = (int) d;
        if(c == D_REPORT_COUNT) r_count = (int) d;

        //Determine the usage
        if(c == D_USAGE_PAGE || c == D_USAGE){
            if(
                d == D_USAGE_BUTTON ||
                d == D_USAGE_WHEEL ||
                d == D_USAGE_X ||
                d == D_USAGE_Y
            ) {
                if(!r_usage_a){
                    r_usage_a = (int) d;
                } else {
                    r_usage_b = (int) d;
                }
            }
        }

        //Check, if we reached the end of this input data type
        if(c == D_INPUT || c == D_FEATURE){
            //Assign usage to data_pos
            if(!button && r_usage_a == D_USAGE_BUTTON){
                data_pos->button.offset = offset;
                data_pos->button.size = r_size*r_count;
                button = 1;
            }
            switch(r_usage_a){
                case D_USAGE_X:
                    data_pos->x.offset = offset;
                    data_pos->x.size = r_size;
                case D_USAGE_Y:
                    data_pos->x.offset = offset;
                    data_pos->x.size = r_size;
            }
             switch(r_usage_b){
                case D_USAGE_X:
                    data_pos->y.offset = offset + r_size;
                    data_pos->y.size = r_size;
                case D_USAGE_Y:
                    data_pos->y.offset = offset + r_size;
                    data_pos->y.size = r_size;
            }
            if(r_usage_a == D_USAGE_WHEEL){
                data_pos->wheel.offset = offset;
                data_pos->wheel.size = r_size*r_count;
            }

            r_usage_a = 0;
            r_usage_b = 0;
            offset += r_size*r_count;
        }
        i += c_len(c);
    }

    return 0;
}

inline int extract_at(unsigned char *data, int data_len, int bit_pos, int bit_size)
{
    int size = bit_size/8;          //Size of our data in bytes
    int i = bit_pos/8;              //Starting index of data[] to access in byte-aligned size
    char shift = bit_pos % 8;       //Remaining bits to shift until we reach our target data
    union {
        unsigned char raw[4];
        unsigned char b1;
        unsigned short b2;
        int i;
    } buffer;

    buffer.i = 0; //Initialized buffer to zero

    //Avoid access violation when using memcpy.
    if(i + size > data_len || (shift && (i + size + 1) > data_len))
        return 0;

    //TODO: Endianess!
    //Right now, we only support 1 byte and 2 byte long data - We explicitly check against the supplied bit-length
    switch(bit_size){
    case 8:
        //Create a local copy, that we can modify. If 'shift' ('pos % 8') was not zero, the buffer must be bigger. 
        //So we extract one more byte. QUESTION: Is this safe? Imagine, the data is 10 bits long and we want to extract bit 2 to 10. We would copy 2 bytes (16 bit),
        //so that we can shift << by 2 bits. What about the remaining 6 bits? Can we even access them? I assume: YES. On this specific architecture x86/x86_64,
        //the shortes data-type is a char of 1 byte (8 bits). And we allocated a buffer for that for DMA access via the usbmouse driver, which itself
        //is then aligned to this minimal size of 1 byte.
        cout << "IN" << endl;
        cout << i << endl;
        memcpy(buffer.raw, data + i, (shift == 0) ? 1 : 2);
        if(shift){
            buffer.i <<= shift;
        }
        return buffer.i;
    }

    return 0; //All other lengths are not supported.
}

// Extracts the interesting mouse data from the raw USB data, according to the layout delcared in the report descriptor
int extract_mouse_events(unsigned char *data, int data_len, struct report_positions *data_pos, int *btn, int *x, int *y, int *wheel)
{
    cout << extract_at(data,data_len,data_pos->wheel.offset, data_pos->wheel.size) << endl;
}

struct steelseries_600_data{
    unsigned char btn;
    signed short x;
    signed short y;
    signed char wheel;
};

int main(){
    //Test parsing of report descriptor
    struct report_positions data_struct;
    parse_report_desc(desc, sizeof(desc)/sizeof(char), &data_struct);
    cout << "Button: Offset " << (unsigned int) data_struct.button.offset << " Size " << (unsigned int) data_struct.button.size << endl;
    cout << "X: Offset " << (unsigned int) data_struct.x.offset << " Size " << (unsigned int) data_struct.x.size << endl;
    cout << "Y: Offset " << (unsigned int) data_struct.y.offset << " Size " << (unsigned int) data_struct.y.size << endl;
    cout << "Wheel: Offset " << (unsigned int) data_struct.wheel.offset << " Size " << (unsigned int) data_struct.wheel.size << endl;

    //Test extraction from a reported data. Here, the "test" data is for a steelseries Rival 600 mouse
    union test_data{
        steelseries_600_data data;
        unsigned char raw[32];          //Big enough buffer
    } test;

    test.data.btn = 19;
    test.data.x = -7;
    test.data.y = 120;
    test.data.wheel = 15;

    int btn, x, y, wheel;

    extract_mouse_events(test.raw, sizeof(test), &data_struct, &btn, &x, &y, &wheel);

    return 0;
}