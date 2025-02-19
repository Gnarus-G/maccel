#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#include "dbg.h"

static inline int64_t div128_s64_s64_s64(int64_t high, int64_t low,
                                         int64_t divisor) {
  int64_t result;
  // s.high.low
  // high -> rdx
  // low -> rax
  uint64_t remainder;
  __asm__("idivq %[B]"
          : "=a"(result), "=d"(remainder)
          : [B] "r"(divisor), "a"(low), "d"(high));

  return result;
}

static inline int is_digit(char c) { return '0' <= c && c <= '9'; }

#endif
