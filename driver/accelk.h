#ifndef _ACCELK_H_
#define _ACCELK_H_

#include "accel.h"
#include "linux/ktime.h"
#include "params.h"

const fixedpt HUNDRED_MS = fixedpt_rconst(100);
const fixedpt US_PER_MS = fixedpt_rconst(1000);

static inline AccelResult accelerate(int x, int y) {
  dbg("FIXEDPT_BITS = %d", FIXEDPT_BITS);

  static ktime_t last_time;
  static s64 last_us = 100; // .1ms

  ktime_t now = ktime_get();
  s64 us = ktime_to_us(now - last_time);

  last_time = now;

  if (us < 100) { // ensure no less than .1ms?
    us = last_us;
  }

  last_us = us;

  fixedpt _us = fixedpt_fromint(us);
  fixedpt ms = fixedpt_div(_us, US_PER_MS);

  if (ms > HUNDRED_MS) { // rounding down to 100 ms
    ms = HUNDRED_MS;
  }

  return f_accelerate(x, y, ms, atofp(PARAM_SENS_MULT), atofp(PARAM_ACCEL),
                      atofp(PARAM_OFFSET), atofp(PARAM_OUTPUT_CAP));
}

#endif // !_ACCELK_H_
