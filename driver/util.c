#include "util.h" 
#include <stdbool.h>

//Converts string into float.
inline void atof(const char* str, int len, float* result){
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
inline void Q_sqrt(float* number)
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
int parse_report_desc(unsigned char* data, int data_len, struct report_structure* data_struct){
    int r_count, r_size, r_usage_a = 0, r_usage_b = 0;
    unsigned char c, d, button;

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
            //Assign usage to data_struct
            if(!button && r_usage_a == D_USAGE_BUTTON){
                data_struct->button.offset = offset;
                data_struct->button.size = r_size*r_count;
                button = 1;
            }
            switch(r_usage_a){
                case D_USAGE_X:
                    data_struct->x.offset = offset;
                    data_struct->x.size = r_size;
                case D_USAGE_Y:
                    data_struct->x.offset = offset;
                    data_struct->x.size = r_size;
            }
             switch(r_usage_b){
                case D_USAGE_X:
                    data_struct->y.offset = offset + r_size;
                    data_struct->y.size = r_size;
                case D_USAGE_Y:
                    data_struct->y.offset = offset + r_size;
                    data_struct->y.size = r_size;
            }
            if(r_usage_a == D_USAGE_WHEEL){
                data_struct->wheel.offset = offset;
                data_struct->wheel.size = r_size*r_count;
            }
            
            r_usage_a = 0;
            r_usage_b = 0;
            offset += r_size*r_count;
        }
        i += c_len(c);
    }

    return 0;
}