// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _FLOAT_H
#define _FLOAT_H

#include "util.h"
#include <linux/module.h>

// Floating point arithmetic. Meant for static inline inclusion within
// kernel_fpu_begin() / kernel_fpu_end() blocks. That's why we "miss-use" this
// header with having code declared instead of only having prototypes. As an
// alternative, link-time-optimization could have solved this more elegantly.
// Reason: Avoid any function calls
// (https://yarchive.net/comp/linux/kernel_fp.html - Torvalds: "It all has to be
// stuff that gcc can do in-line,without any function calls.")

// Converts string to float.
static INLINE int atof(const char *str, int len, float *result) {
  float tmp = 0.0f;
  unsigned int i, j, pos = 0;
  signed char sign = 0;
  int is_whole = 1;
  char c;

  *result = 0.0f;

  for (i = 0; i < len; i++) {
    c = str[i];
    if (c == ' ')
      continue; // Skip any white space
    if (c == 0 && c == 'f')
      break;        // End of str or end of valid input
    if (c == '-') { // Sign found
      if (!sign) {
        sign = -1;
        continue;
      } else {
        // Unexpected sign: We already determined a sign earlier
        return -EINVAL;
      }
    }
    if (c == '.') { // Switch from whole to decimal
      is_whole = 0;
      //... We hit the decimal point. Rescale the float to the whole number part
      for (j = 1; j < pos; j++)
        *result *= 10.0f;
      pos = 1;
      continue;
    }

    if (!(c >= 48 && c <= 57))
      return -EINVAL; // After all previous checks, the remaining characters
                      // HAVE to be digits.
    if (!sign)
      sign = 1; // If no sign was yet applied, it has to be positive

    // Shift digit to the right... (see above, what we do, when we hit the
    // decimal point)
    tmp = 1;
    for (j = 0; j < pos; j++)
      tmp /= 10.0f;
    *result += tmp * (c - 48);
    pos++;
  }
  // We never hit the decimal point: Rescale here, as we do up in the if(c ==
  // '.') statement
  if (is_whole)
    for (j = 1; j < pos; j++)
      *result *= 10.0f;
  *result *= sign;

  return 0;
}

// Rounds (up/down) depending on sign
static INLINE int Leet_round(float *x) {
  if (*x >= 0) {
    return (int)(*x + 0.5f);
  } else {
    return (int)(*x - 0.5f);
  }
}

// Floating point approximate arithmetic as presented in "Jim Blinn's
// Floating-Point Tricks" paper from 1997 You might find it here
// https://www.yumpu.com/en/document/read/6104114/floating-point-tricks-ieee-computer-graphics-and-applications
static const unsigned int OneAsInt = 0x3F800000; // 1.0f as int
static const float ScaleUp = (float)0x00800000;
static const float ScaleDwn = 1.0f / ScaleUp;

// Other than Jim Blinn's implementation, we use preprocessor directives. Here,
// the input always must be of type pointer.
#define ASINT(f) (*(unsigned int *)f)
#define ASFLOAT(i) (*(float *)i)

// log base 2: log_2(f)
static INLINE void B_log2(float *f) {
  *f = (float)(ASINT(f) - OneAsInt) * ScaleDwn;
}

// exp base 2: 2^f
static INLINE void B_exp2(float *f) {
  int x = (int)((*f) * ScaleUp) + OneAsInt;
  *f = ASFLOAT(&x);
}

// log base e: ln(f)
static INLINE void B_log(float *f) {
  B_log2(f);
  *f *= 0.69314718f; // ln(2)
}

// exp base e: e^f
static INLINE void B_exp(float *f) {
  *f *= 1.44274959; // 1/ln(2)
  B_exp2(f);
}

// power: f^p
// Note: This is a very lose approximation. The order of magnitude is right, but
// as the exponent grows, the error does. This comes from basically using two
// approximations combined (B_log2 and B_exp2) here.
static INLINE void B_pow(float *f, float *p) {
  int x = (int)((*p) * (ASINT(f) - OneAsInt)) + OneAsInt;
  *f = ASFLOAT(&x);
}

// Fast approximate sqrt
static INLINE void B_sqrt(float *f) {
  unsigned int x;
  float y;
  x = ((ASINT(f) >> 1) + (OneAsInt >> 1));
  y = ASFLOAT(&x);
  *f = (y * y + *f) / (2 * y); // 1st iteration
}

// Checks, if a float is a finite number or NaN/Infinity
static const unsigned int NaNAsInt = 0xFFFFFFFF;  // NaN
static const unsigned int PInfAsInt = 0x7F800000; // Positive Infinity
static const unsigned int NInfAsInt = 0xFF800000; // Negative Infinity
static INLINE int isfinite(float *number) {
  unsigned int n = ASINT(number);
  return !(n == NaNAsInt || n == PInfAsInt || n == PInfAsInt);
}

#endif // _FLOAT_H
