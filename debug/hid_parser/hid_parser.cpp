#include <iostream>
using namespace std;

//#include "linux/string.h" //Necessary for memcpy to work inside kernel module
#include <cstring>          //memcpy
#include <bitset>
#include <linux/kernel.h>
#include "hid_parser.h"

//Dummy
#define le16_to_cpu(x) x

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


//This is the most crudest HID descriptor parser EVER.
//We will skip most control words until we found an interesting one
//We also assume, that the first button-definition we will find is the most important one,
//so we will ignore any further button definitions

struct parser_context {
    unsigned char id;                           // Report ID
    unsigned int offset;                        // Local offset in this report ID context
};

#define NUM_CONTEXTS 32                             // This should be more than enough for a HID mouse. If we exceed this number, the parser below will eventually fail
int parse_report_desc(unsigned char *buffer, int buffer_len, struct report_positions *pos)
{
    int r_count = 0, r_size = 0, len = 0;
    int r_usage[2];
    unsigned char ctl, data, button = 0;

    unsigned int n, i = 0;

    //Parsing contexts are linked to the Report ID (if declared) in the report descriptor
    int context_found;
    struct parser_context contexts[NUM_CONTEXTS];    // We allow up to NUM_CONTEXTS different parsing contexts. Any further will be ignored.
    struct parser_context *c = contexts;             // The current context

    r_usage[0] = 0;
    r_usage[1] = 0;
    pos->report_id_tagged = 0;

    //Initialize contexts to zero
    for(n = 0; n < NUM_CONTEXTS; n++){
        contexts[n].id = 0;
        contexts[n].offset = 0;
    }

    while(i < buffer_len){
        ctl = buffer[i] & 0xFC;                     // Control word with the length-bits stripped
        len = buffer[i] & 0x03;                     // Length of the the proceeding data, following the control word (in bytes)
        if(i < buffer_len) data = buffer[i+1];      // Beginning of data after the control word

        // ######## Global items
        //Determine the size
        if(ctl == D_REPORT_SIZE)  r_size = (int) data;
        if(ctl == D_REPORT_COUNT) r_count = (int) data;

        //Switch context, if a "Report ID" control word has been found.
        if(ctl == D_REPORT_ID){
            pos->report_id_tagged = 1;
            // Search all available contexts for a match...
            context_found = 0;
            for(n = 0; n < NUM_CONTEXTS; n++){
                if(contexts[n].id == data){
                    c = contexts + n;
                    c->id = data;
                    context_found = 1;
                    break;
                }
            }
            // No existing parsing context matching the Report ID found. Create a new one!
            if(!context_found){
                for(n = 0; n < NUM_CONTEXTS; n++){
                    if(contexts[n].id == 0){
                        c = contexts + n;
                        c->id = data;
                        c->offset = 8;              // Since we use a Report ID , which preceeds the actual report (1 byte), all offsets are shifted
                        break;
                    }
                }
            }
        }

        // ######## Local items
        //Determine standard usage
        if((ctl == D_USAGE_PAGE || ctl == D_USAGE) && len == 1){
            if(
                data == D_USAGE_BUTTON ||
                data == D_USAGE_WHEEL ||
                data == D_USAGE_X ||
                data == D_USAGE_Y
            ) {
                if(!r_usage[0]){
                    r_usage[0] = (int) data;
                } else {
                    r_usage[1] = (int) data;
                }
            }
        }

        // ######## Main items
        //Check, if we reached the end of this input data type
        if(ctl == D_INPUT || ctl == D_FEATURE){
            //Assign usage to pos
            if(!button && r_usage[0] == D_USAGE_BUTTON){
                pos->button.id = c->id;
                pos->button.offset = c->offset;
                pos->button.size = r_size*r_count;
                button = 1;
            }
            for(n = 0; n < 2; n++){
                switch(r_usage[n]){
                case D_USAGE_X:
                    pos->x.id = c->id;
                    pos->x.offset = c->offset + r_size*n;
                    pos->x.size = r_size;
                    break;
                case D_USAGE_Y:
                    pos->y.id = c->id;
                    pos->y.offset = c->offset + r_size*n;
                    pos->y.size = r_size;
                    break;
                }

            }
            if(r_usage[0] == D_USAGE_WHEEL){
                pos->wheel.id = c->id;
                pos->wheel.offset = c->offset;
                pos->wheel.size = r_size*r_count;
            }
            //Reset local tags
            r_usage[0] = 0;
            r_usage[1] = 0;
            //Increment offset
            c->offset += r_size*r_count;
        }
        i += len + 1;
    }

    return 0;
}

//Shifts an array *data of byte-length data_len by amounts of +/- num bits to the right/left (limited to num = +/-8 max).
//Most certainly not the most elegant way to do this. However, it works.
inline void array_shift(unsigned char *data, int data_len, int num){
    int i;
    if(num == 0) return;

    if(num < 0){
        num *= -1;
        for(i = 0; i < data_len; i++){
            data[i] <<= num;
            if(i + 1 < data_len){
                data[i] |= data[i+1] >> (8 - num);
            }
        }
    } else {
        for(i = data_len - 1; i >= 0; i--){
            data[i] >>= num;
            if(i){
                data[i] |= data[i-1] << (8 - num);
            }
        }
    }
}

//Extracts a number from a raw USB stream, according to its bit_pos and bit_size
inline int extract_at(unsigned char *data, int data_len, int bit_pos, int bit_size)
{
    int size = bit_size/8;          //Size of our data in bytes
    int i = bit_pos/8;              //Starting index of data[] to access in byte-aligned size
    char shift = bit_pos % 8;       //Remaining bits to shift left, until we reach our target data
    union {
        __u8 raw[4];    //Raw buffer of individual bytes. Must be of same length as "con" and at least 1 byte bigger than the biggest datatype you want to handle in here
        __u32 init;     //Continous buffer of aboves bytes (used for initialization)
        __s8 b1;        //Return value
        __s16 b2;       //Return value
    } buffer;

    //Data structure to read is bigger than a clear multiple of 8 bits. Read one more byte.
    if(bit_size % 8) size += 1;
    //Data structure to read is bigger than we can handle. Abort
    if(size > sizeof(buffer.init)) return 0;

    buffer.init = 0; //Initialized buffer to zero

    //Avoid access violation when using memcpy.
    if(i + size > data_len) return 0;

    //Create a local copy, that we can modify.
    memcpy(buffer.raw, data + i, size);

    if(shift)
        array_shift(buffer.raw,size,-1*shift);              //Truncate bits, that we copied over too much on the right

    if(bit_size <= 8){
        if(shift)
            buffer.raw[0] &= (0xFF << shift);               //Mask bits, which do not belong here
        return (int) buffer.b1;
    }
    if(bit_size <= 16){
        if(shift)
            buffer.raw[1] &= (0xFF << shift);               //Mask bits, which do not belong here
        return (int) buffer.b2;
    }

    return 0; //All other lengths are not supported.
}

// Extracts the interesting mouse data from the raw USB data, according to the layout delcared in the report descriptor
int extract_mouse_events(unsigned char *buffer, int buffer_len, struct report_positions *pos, int *btn, int *x, int *y, int *wheel)
{
    unsigned char id = 0;
    if(pos->report_id_tagged)
        id = buffer[0];

    /*
    int i;
    printk(KERN_CONT "Raw: ");
    for(i = 0; i<buffer_len;i++){
        printk(KERN_CONT "%x ", (int) buffer[i]);
    }
    printk(KERN_CONT "\n");
    */

    *btn = 0; *x = 0; *y = 0; *wheel = 0;
    if(pos->button.id == id)
        *btn =      extract_at(buffer, buffer_len, pos->button.offset, pos->button.size);
    if(pos->x.id == id)
        *x =        extract_at(buffer, buffer_len, pos->x.offset,      pos->x.size);
    if(pos->y.id == id)
        *y =        extract_at(buffer, buffer_len, pos->y.offset,      pos->y.size);
    if(pos->wheel.id == id)
        *wheel =    extract_at(buffer, buffer_len, pos->wheel.offset,  pos->wheel.size);

    return 0;
// }

struct steelseries_600_data{
    unsigned char btn;
    signed short x;
    signed short y;
    signed char wheel;
} __attribute__((packed));

int main(){
    //Test parsing of report descriptor
    struct report_positions data_struct;
    parse_report_desc(desc, sizeof(desc)/sizeof(char), &data_struct);

    cout << "Is tagged with report ID: " << data_struct.report_id_tagged << endl;

    cout << "Button ("<< (unsigned int) data_struct.button.id << "): Offset " << (unsigned int) data_struct.button.offset << " Size " << (unsigned int) data_struct.button.size << endl;
    cout << "X: Offset ("<< (unsigned int) data_struct.x.id << ") " << (unsigned int) data_struct.x.offset << " Size " << (unsigned int) data_struct.x.size << endl;
    cout << "Y: Offset ("<< (unsigned int) data_struct.y.id << ") " << (unsigned int) data_struct.y.offset << " Size " << (unsigned int) data_struct.y.size << endl;
    cout << "Wheel: Offset ("<< (unsigned int) data_struct.wheel.id << ") " << (unsigned int) data_struct.wheel.offset << " Size " << (unsigned int) data_struct.wheel.size << endl;

    //Test extraction from a reported data. Here, the "test" data is for a steelseries Rival 600 mouse
    union test_data{
        struct steelseries_600_data data;
        unsigned char raw[32];          //Big enough buffer
    } test;

    //Apply some test-data to be extracted by extract_mouse_events
    test.data.btn = 19;
    test.data.x = -7;
    test.data.y = 120;
    test.data.wheel = 15;

    int btn, x, y, wheel;

    extract_mouse_events(test.raw, sizeof(test), &data_struct, &btn, &x, &y, &wheel);

    cout << "Button: " << btn << endl;
    cout << "X: " << x << endl;
    cout << "Y: " << y << endl;
    cout << "Wheel: " << wheel << endl;

    cout << (0x80 & 0x03) << endl;

    return 0;
}
