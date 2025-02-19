#include <linux/types.h>

#ifndef __KERNEL__
#include <stdint.h>
#endif // !__KERNEL__

/**
 * [Yeetmouse](https://github.com/AndyFilter/YeetMouse/blob/master/driver/FixedMath/Fixed64.h#L1360)
 * has better string utils. FP64_ToString there doesn't require
 * `__udivmodti4` in 64-bit mode.
 *
 */

typedef int32_t FP_INT;
typedef int64_t FP_LONG;

#define FP64_Shift 32

static inline FP_LONG FP64_Mul(FP_LONG a, FP_LONG b) {
  return (FP_LONG)(((__int128_t)a * (__int128_t)b) >> FP64_Shift);
}

static char *FP_64_itoa_loop(char *buf, uint64_t scale, uint64_t value,
                             int skip) {
  while (scale) {
    unsigned digit = (value / scale);

    if (!skip || digit || scale == 1) {
      skip = 0;
      *buf++ = '0' + digit;
      value %= scale;
    }

    scale /= 10;
  }
  return buf;
}

void FP64_ToString(FP_LONG value, char *buf);

void FP64_ToString(FP_LONG value, char *buf) {
  uint64_t uvalue = (value >= 0) ? value : -value;
  if (value < 0)
    *buf++ = '-';

#define SCALE 9

  static const uint64_t FP_64_scales[10] = {
      /* 18 decimals is enough for full 64bit fixed point precision */
      1,      10,      100,      1000,      10000,
      100000, 1000000, 10000000, 100000000, 1000000000};

  /* Separate the integer and decimal parts of the value */
  uint64_t intpart = uvalue >> 32;
  uint64_t fracpart = uvalue & 0xFFFFFFFF;
  uint64_t scale = FP_64_scales[SCALE];
  fracpart = FP64_Mul(fracpart, scale);

  if (fracpart >= scale) {
    /* Handle carry from decimal part */
    intpart++;
    fracpart -= scale;
  }

  /* Format integer part */
  buf = FP_64_itoa_loop(buf, 1000000000, intpart, 1);

  /* Format decimal part (if any) */
  if (scale != 1) {
    *buf++ = '.';
    buf = FP_64_itoa_loop(buf, scale / 10, fracpart, 0);
  }

  *buf = '\0';
}
