#ifndef _UTILS_H_
#define _UTILS_H_

#include "accel.h"
#include "fixedptc.h"

static inline int is_digit(char c) { return '0' <= c && c <= '9'; }

static fixedpt atofp(char *num_string) {
  fixedptud n = 0; // just looking to assign eith u64 or u128 depending on
                   // fixedpt settings

  for (int idx = 0; num_string[idx] != '\0'; idx++) {
    char c = num_string[idx];
    switch (c) {
    case ' ':
      continue;
    default:
      if (is_digit(c)) {
        int digit = c - '0';
        n = n * 10 + digit;
      } else {
        dbg("Hit an unsupported character while parsing a number: '%c'", c);
      }
    }
  }

  return n;
}

#endif
