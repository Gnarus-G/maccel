#include "util.h"
#include <linux/kernel.h>   //fixed-len datatypes
#include <linux/string.h>   //memcpy

//Converts string into float.
inline void atof(const char *str, int len, float *result)
{
    float tmp = 0.0f;
    unsigned int i, j, pos = 0;
    signed char sign = 0;
    bool is_whole = 0;
    char c;

    *result = 0.0f;

    for(i = 0; i < len; i++){
        c = str[i];
        if(c == ' ') continue;              //Skip any white space
        if(c == 0) break;                   //End of str
        if(!sign && c == '-'){              //Sign is negative
            sign = -1;
            continue;
        }
        if(c == '.'){                       //Switch from whole to decimal
            is_whole = true;
            //... We hit the decimal point. Rescale the float to the whole number part
            for(j = 1; j < pos; j++) *result *= 10.0f;
            pos = 1;
            continue;
        }

        if(!(c >= 48 && c <= 57)) break;    //After all previous checks, the remaining characters HAVE to be digits. Otherwise break
        if(!sign) sign = 1;                 //If no sign was yet applied, it has to be positive
        
        //Shift digit to the right... (see above, what we do, when we hit the decimal point)
        tmp = 1;
        for(j = 0; j < pos; j++) tmp /= 10.0f;
        *result += tmp*(c-48);
        pos++;
    }
    //We never hit the decimal point: Rescale here, as we do up in the if(c == '.') statement
    if(is_whole)
        for(j = 1; j < pos; j++) *result *= 10.0f;
    *result *= sign;
}

// Rounds (up/down) depending on sign
inline int Leet_round(float x)
{
    if (x >= 0) {
        return (int)(x + 0.5f);
    } else {
        return (int)(x - 0.5f);
    }
}

// What do we have here? Code from Quake 3, which is also GPL.
// https://en.wikipedia.org/wiki/Fast_inverse_square_root
// Copyright (C) 1999-2005 Id Software, Inc.
inline void Q_sqrt(float *number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = (*number) * 0.5F;
    y  = (*number);
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    *number = 1 / y;
}

//Length of "ctl word + data"
inline int c_len(const unsigned char c)
{
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
                break;
            case D_USAGE_Y:
                data_pos->x.offset = offset;
                data_pos->x.size = r_size;
                break;
            }
            switch(r_usage_b){
            case D_USAGE_X:
                data_pos->y.offset = offset + r_size;
                data_pos->y.size = r_size;
                break;
            case D_USAGE_Y:
                data_pos->y.offset = offset + r_size;
                data_pos->y.size = r_size;
                break;
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

//Extracts a number from a raw USB stream, according to its bit_pos and bit_size
inline int extract_at(unsigned char *data, int data_len, int bit_pos, int bit_size)
{
    int size = bit_size/8;          //Size of our data in bytes
    int i = bit_pos/8;              //Starting index of data[] to access in byte-aligned size
    char shift = bit_pos % 8;       //Remaining bits to shift until we reach our target data
    union {
        __u8 raw[4];    //Raw buffer of individual bytes. Must be of same length as "con" and at least 1 byte bigger than the biggest datatype you want to handle in here
        __u32 con;      //Continous buffer of aboves bytes (used for bitshiftig)
        __s8 b1;        //Return value
        __s16 b2;       //Return value
    } buffer;

    if(size > sizeof(buffer.con)) return 0;

    buffer.con = 0; //Initialized buffer to zero

    //Avoid access violation when using memcpy.
    if(i + size > data_len || (shift && (i + size + 1) > data_len))
        return 0;

    //Create a local copy, that we can modify. If 'shift' ('pos % 8') was not zero, copy an additional byte.
    memcpy(buffer.raw, data + i, (shift == 0) ? size : size + 1);

    // Respect byte-order
    // A BIG TODO/Questions: If we need to shift bits from the raw Little-Endian USB datastream on a Big-Endian system, do we first need to convert the
    // byte-order and THEN shift bits, or do we need to first shift the bits and then arrange the byte-order? I assume the first: We copy the raw
    // LE data-stream from the USB device in a __u8 buffer array. Via the union, this is now overlayed on a ´multi-byte machine´ datatype. Since this
    // ´machine´ applies bit-shifts on multi-byte values as "its architecture dictates", we first need to convert LE -> machine-type and then bitshift.

    //TODO: Find out...
    //buffer.con = le32_to_cpu(buffer.con);

    if(shift) buffer.con <<= shift;

    //TODO: Endianess!
    //Right now, we only support 1 byte and 2 byte long data - We explicitly check against the supplied bit-length
    switch(bit_size){
    case 8:
        return (int) buffer.b1;
    case 16:
        return (int) buffer.b2;
    }

    return 0; //All other lengths are not supported.
}

// Extracts the interesting mouse data from the raw USB data, according to the layout delcared in the report descriptor
int extract_mouse_events(unsigned char *data, int data_len, struct report_positions *data_pos, int *btn, int *x, int *y, int *wheel)
{
    *btn =      extract_at(data, data_len, data_pos->button.offset, data_pos->button.size);
    *x =        extract_at(data, data_len, data_pos->x.offset,      data_pos->x.size);
    *y =        extract_at(data, data_len, data_pos->y.offset,      data_pos->y.size);
    *wheel =    extract_at(data, data_len, data_pos->wheel.offset,  data_pos->wheel.size);

    return 0;
}
