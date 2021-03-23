#include "util.h" 
#include <stdbool.h>

//Converts string into float.
void atof(const char* str, int len, float* result){
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
            for(j = 1; j < pos; j++) *result *= 10;
            pos = 1;
            continue;
        }

        if(!(c >= 48 && c <= 57)) break;    //After all previous checks, the remaining characters HAVE to be digits. Otherwise break
        if(!sign) sign = 1;                 //If no sign was yet applied, it has to be positive
        
        //Shift digit to the right... (see above, what we do, when we hit the decimal point)
        tmp = 1;
        for(j = 0; j < pos; j++) tmp /= 10;
        *result += tmp*(c-48);
        pos++;
    }
    //We never hit the decimal point: Rescale here, as we do up in the if(c == '.') statement
    if(is_whole)
        for(j = 1; j < pos; j++) *result *= 10;
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
void Q_sqrt(float* number)
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
