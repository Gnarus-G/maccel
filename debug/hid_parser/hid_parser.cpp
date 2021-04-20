#include <iostream>
using namespace std;

//#include "linux/string.h" //Necessary for memcpy to work inside kernel module
#include <cstring>          //memcpy
#include <bitset>
#include <linux/kernel.h>
#include "hid_parser.h"

//This is supposed to be in linux/kernel.h. However, it is not within the scope when compiling this test-program here.
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//Dummy
#define le16_to_cpu(x) x
#define le32_to_cpu(x) x
#define be16_to_cpu(x) (x >> 8) | (x << 8)

//This is the most crudest HID descriptor parser EVER.
//We will skip most control words until we found an interesting one
//We also assume, that the first button-definition we will find is the most important one,
//so we will ignore any further button definitions

struct parser_context {
    unsigned char id;                           // Report ID
    unsigned int offset;                        // Local offset in this report ID context
};

#define NUM_CONTEXTS 32                             // This should be more than enough for a HID mouse. If we exceed this number, the parser below will eventually fail
#define SET_ENTRY(entry, _id, _offset, _size, _sign) \
    entry.id = _id;                                 \
    entry.offset = _offset;                         \
    entry.size = _size;                             \
    entry.sgn = _sign;

int parse_report_desc(unsigned char *buffer, int buffer_len, struct report_positions *pos)
{
    int r_count = 0, r_size = 0, r_sgn = 0, len = 0;
    int r_usage[16];
    unsigned char ctl, button = 0;
    unsigned char *data;

    unsigned int n, i = 0;

    //Parsing contexts are activated by the  "Report ID" tag. The parser will switch between contexts, when it sees this keyword.
    int context_found;
    struct parser_context contexts[NUM_CONTEXTS];    // We allow up to NUM_CONTEXTS different parsing contexts. Any further will be ignored.
    struct parser_context *c = contexts;             // The current context

    for(n = 0; n < ARRAY_SIZE(r_usage); n++){
        r_usage[n] = 0;
    }
    
    pos->report_id_tagged = 0;

    //Initialize contexts to zero
    for(n = 0; n < NUM_CONTEXTS; n++){
        contexts[n].id = 0;
        contexts[n].offset = 0;
    }

    while(i < buffer_len){
        ctl = buffer[i] & 0xFC;                     // Control word with the length-bits stripped
        len = buffer[i] & 0x03;                     // Length of the the proceeding data, following the control word (in bytes)
        if(i < buffer_len) data = buffer + i + 1;   // Beginning of data after the control word

        // ######## Global items
        //Determine the size
        if(ctl == D_REPORT_SIZE)  r_size = (int) data[0];
        if(ctl == D_REPORT_COUNT) r_count = (int) data[0];

        //Switch context, if a "Report ID" control word has been found.
        if(ctl == D_REPORT_ID){
            pos->report_id_tagged = 1;
            // Search all available contexts for a match...
            context_found = 0;
            for(n = 0; n < NUM_CONTEXTS; n++){
                if(contexts[n].id == data[0]){
                    c = contexts + n;
                    c->id = data[0];
                    context_found = 1;
                    break;
                }
            }
            // No existing parsing context matching the Report ID found. Create a new one!
            if(!context_found){
                for(n = 0; n < NUM_CONTEXTS; n++){
                    if(contexts[n].id == 0){
                        c = contexts + n;
                        c->id = data[0];
                        c->offset = 8;              // Since we use a Report ID , which preceeds the actual report (1 byte), all offsets are shifted
                        break;
                    }
                }
            }
        }

        //Determine sign.
        if(ctl == D_LOGICAL_MINIMUM){
            switch(len){
            case 1:
                r_sgn = ((int) *((__s8*) data)) < 0;
                break;
            case 2:
                r_sgn = (__s16) le16_to_cpu(*((__s16*) data)) < 0;
                break;
            //case 4:
            //Sure, we could also now check 4 bytes, as it is written down in the HID specs...
            //However, my extract_at function does not support 4-byte numbers and I don't see, why I should implement it. A mouse should never need to send 4-bytes long messages IMHO.
            }
        }

        // ######## Local items (sort of...) - While a button is described via a global Usage Page (Button), other controls like the Wheel or Pointer Axis are described via local 'Usage' tags.
        //Determine standard usage
        if((ctl == D_USAGE_PAGE || ctl == D_USAGE) && len == 1){
            if(
                data[0] == D_USAGE_BUTTON ||
                data[0] == D_USAGE_WHEEL ||
                data[0] == D_USAGE_X ||
                data[0] == D_USAGE_Y
            ) {
                for(n = 0; n < ARRAY_SIZE(r_usage); n++){
                    if(!r_usage[n]){
                        r_usage[n] = (int) data[0];
                        break;
                    }
                }
            }
        }

        // ######## Main items
        //Check, if we reached the end of this input data type
        if(ctl == D_INPUT || ctl == D_FEATURE){
            //Buttons are handled separately
            if(!button && r_usage[0] == D_USAGE_BUTTON){
                SET_ENTRY(pos->button, c->id, c->offset, r_size*r_count, r_sgn);
                button = 1;
            } else {
            //X,Y and WHEEL
                for(n = 0; n < r_count; n++){
                    switch(r_usage[n]){
                    case D_USAGE_X:
                        SET_ENTRY(pos->x, c->id, c->offset + r_size*n, r_size, r_sgn);
                        break;
                    case D_USAGE_Y:
                        SET_ENTRY(pos->y, c->id, c->offset + r_size*n, r_size, r_sgn);
                        break;
                    case D_USAGE_WHEEL:
                        SET_ENTRY(pos->wheel, c->id, c->offset + r_size*n, r_size, r_sgn);
                        break;
                    }

                }
            }
            //Reset usages
            for(n = 0; n < ARRAY_SIZE(r_usage); n++){
                r_usage[n] = 0;
            }
            //Increment offset
            c->offset += r_size*r_count;
        }
        i += len + 1;
    }

    return 0;
}

//Shifts an array *data of length data_len (in bytes) by amounts of num in bits to the right/left, depending on right = 1/0 (maximum bits to shift: 8)
//This shift method is assuming little-endian data, as it is dictated by the HID standard.
//So taking a byte as [MSB, ..., LSB], then e.g. a 2 byte long array "[7,8,6,5,4,3,2,1,0], [15,14,13,12,11,10,9] << 2" (left = 1, num = 2) becomes "[5,4,3,2,1,0,_,_], [13,12,11,10,9,8,7,6]"
//Most certainly not the most elegant and fastest way to do this. However, it works. (Limiting this to only 4-byte long data, one could use a preprocessor define, which is way faster but bound to that type).
inline void array_shift_le(unsigned char *data, int data_len, bool right, int num){
    int i;
    if(num == 0) return;

    //Right shift
    if(right){
        for(i = 0; i < data_len; i++){
            data[i] >>= num;
            if(i + 1 < data_len){
                data[i] |= data[i+1] << (8 - num);
            }
        }
    //Left shift
    } else {
        for(i = data_len - 1; i >= 0; i--){
            data[i] <<= num;
            if(i){
                data[i] |= data[i-1] >> (8 - num);
            }
        }
    }
}

//Extracts a number from a raw USB stream, according to its bit-position and bit-size as stated in the report_entry
inline int extract_at(unsigned char *data, int data_len, struct report_entry *entry)
{
    int size = entry->size/8;           //Size of our data in bytes
    int i = entry->offset/8;            //Starting index of data[] to access in byte-aligned size
    char shift = entry->offset % 8;     //Remaining bits to shift left, until we reach our target data
    union {
        __u8 raw[4];    //Raw buffer of individual bytes. Must be of same length as "init" and at least 2 bytes bigger than the biggest datatype you want to handle in here
        __u32 init;     //Continous buffer of aboves bytes (used for initialization)
        __s8 s8;        //Return value
        __s16 s16;      //Return value
        __u8 u8;        //Return value
        __u16 u16;      //Return value
    } buffer;

    //Data structure to read is bigger than a clear multiple of 8 bits. Read one more byte.
    if(entry->size % 8) size += 1;
    //The bit position is not aligned with byte-size. Read one more byte to make sure, we catch all necessary bits
    if(shift) size += 1;
    //Data structure to read is bigger than we can handle. Abort
    if(size > sizeof(buffer.init)) return 0;

    buffer.init = 0; //Initialized buffer to zero

    //Avoid access violation when using memcpy.
    if(i + size > data_len) return 0;

    //Create a local copy, that we can modify.
    memcpy(buffer.raw, data + i, size);

    //NOTE: All data manipulation below is only valid for little-endian data. The HID standard however dictates little-endian, so this is fine. However, we need to transform to machine data in the end for multi-byte types
    if(shift)
        array_shift_le(buffer.raw,size,true,shift);         //Align bits to memory

    if(entry->size <= 8){
        buffer.raw[0] &= 0xFF >> (8 - entry->size);         //Zero MSB bits, if entry->size < 8
        if(entry->sgn)                                      //If the initial value was signed and if it was not exactly of size 8, we need to add the missing sign-bits, which have been masked above with zeros.
            buffer.raw[0] = buffer.raw[0] >> (8 - entry->size - 1) == 0 ? buffer.raw[0] : (0xFF ^ (0xFF >> (8 - entry->size))) | buffer.raw[0];
        
        return (int) (entry->sgn ? buffer.s8 : buffer.u8);
    }
    if(entry->size <= 16){
        buffer.raw[1] &= 0xFF >> (16 - entry->size);        //Zero MSB bits, if entry->size < 16
        if(entry->sgn)                                      //If the initial value was signed and if it was not exactly of size 16, we need to add the missing sign-bits, which have been masked above with zeros.
            buffer.raw[1] = buffer.raw[1] >> (16 - entry->size - 1) == 0 ? buffer.raw[1] : (0xFF ^ (0xFF >> (16 - entry->size))) | buffer.raw[1];

        buffer.u16 = le16_to_cpu(buffer.u16);               //Convert to machine units
        return (int) (entry->sgn ? buffer.s16 : buffer.u16);
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
        *btn =      extract_at(buffer, buffer_len, &pos->button);
    if(pos->x.id == id)
        *x =        extract_at(buffer, buffer_len, &pos->x);
    if(pos->y.id == id)
        *y =        extract_at(buffer, buffer_len, &pos->y);
    if(pos->wheel.id == id)
        *wheel =    extract_at(buffer, buffer_len, &pos->wheel);

    return 0;
}

struct steelseries_600_data{
    unsigned char btn;
    signed short x;
    signed short y;
    signed char wheel;
} __attribute__((packed));

// Report descriptor we use for debugging below
unsigned char desc[] =  {
    TRUST_GXT
    //SWIFTPOINT_TRACER
    //COOLERMASTER_MM710
    //LOGITECH_G5
    //CSL_OPTICAL_MOUSE
    //STEELSERIES_RIVAL_600
};

//Raw packets from one of the mice I tested
unsigned char packet[] = {
    //### CSL Optical Mouse
    
    //0x01, 0x00, 0xfd, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    
    //0x01, 0x00, 0x00, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    0x01, 0x00, 0x04, 0xd0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    //0x01, 0x00, 0xfe, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    //### Steelseries Rival 600
    //0x00, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


#define DBG(pre, entry) \
    cout << pre << "\t(" << (unsigned int) entry.id << "): Offset " << (unsigned int) entry.offset << "\tSize " << (unsigned int) entry.size << "\tSigned " << (unsigned int) entry.sgn << endl;
int main(){
    //Test parsing of report descriptor
    struct report_positions pos;
    parse_report_desc(desc, sizeof(desc)/sizeof(char), &pos);

    cout << "Is tagged with report ID: " << (pos.report_id_tagged ? "Yes" : "No") << endl;

    DBG("BTN",pos.button);
    DBG("X",pos.x);
    DBG("Y",pos.y);
    DBG("WHL",pos.wheel);

    //Test extraction from a reported data. Here, the "test" data is for a steelseries Rival 600 mouse
    union test_data{
        struct steelseries_600_data data;
        unsigned char raw[32];          //Big enough buffer
    } test;

    //Apply some test-data to be extracted by extract_mouse_events for a steelseries rival 600
    test.data.btn = 19;
    test.data.x = -7;
    test.data.y = 120;
    test.data.wheel = 15;

    int btn, x, y, wheel;

    //extract_mouse_events(test.raw, sizeof(test), &data_struct, &btn, &x, &y, &wheel);
    extract_mouse_events(packet, sizeof(packet), &pos, &btn, &x, &y, &wheel);

    union {
        int i;
        unsigned char c[4];
    } tmp;

    cout << "Bits: " << endl;
    for(int i = 0; i < sizeof(packet); i++){
        cout << bitset<8>(packet[i]) << " ";
    }
    cout << endl;

    tmp.i = x;
    for(int i = 0; i < 4; i++){
        cout << bitset<8>(tmp.c[i]) << " ";
    }
    cout << endl;

    tmp.i = y;
    for(int i = 0; i < 4; i++){
        cout << bitset<8>(tmp.c[i]) << " ";
    }
    cout << endl;

    cout << "BTN:\t" << btn << endl;
    cout << "X:\t" << x << endl;
    cout << "Y:\t" << y << endl;
    cout << "WHL:\t" << wheel << endl;

    return 0;
}
