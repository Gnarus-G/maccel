#ifndef _ACCELK_H_
#define _ACCELK_H_

#include "accel.h"
#include "fixedptc.h"
#include "linux/ktime.h"
#include "params.h"

#if FIXEDPT_BITS == 64
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000000); // 1 million nanoseconds
#else
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000); // 1 thousand microsends
#endif

static inline AccelResult accelerate(int x, int y) {
  dbg("FIXEDPT_BITS = %d", FIXEDPT_BITS);

  static ktime_t last_time;
  ktime_t now = ktime_get();

#if FIXEDPT_BITS == 64
  s64 unit_time = ktime_to_ns(now - last_time);
  dbg("ktime interval -> now (%llu) vs last_ktime (%llu), diff = %llins", now,
      last_time, unit_time);
#else
  s64 unit_time = ktime_to_us(now - last_time);
  dbg("ktime interval -> now (%llu) vs last_ktime (%llu), diff = %llius", now,
      last_time, unit_time);
#endif
  last_time = now;

  fixedpt _unit_time = fixedpt_fromint(unit_time);
  fixedpt millisecond = fixedpt_div(_unit_time, UNIT_PER_MS);

#if FIXEDPT_BITS == 64
  dbg("ktime interval -> converting to ns: %lluns -> %sms", unit_time,
      fptoa(millisecond));
#else
  dbg("ktime interval, converting to us: %lluus -> %sms", unit_time,
      fptoa(millisecond));
#endif

  return f_accelerate(x, y, millisecond, atofp(PARAM_SENS_MULT),
                      atofp(PARAM_ACCEL), atofp(PARAM_OFFSET),
                      atofp(PARAM_OUTPUT_CAP));
}

#endif // !_ACCELK_H_
