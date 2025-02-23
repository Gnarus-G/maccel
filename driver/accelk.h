#ifndef _ACCELK_H_
#define _ACCELK_H_

#include "accel.h"
#include "fixedptc.h"
#include "linux/ktime.h"
#include "params.h"

const fixedpt HUNDRED_MS = fixedpt_rconst(100);

// To support up to 8000Hz polling rate
// we should cap the floor to 0.125ms.
// This is an arbitrary decision...
#if FIXEDPT_BITS == 64
const s64 EIGTH_OF_A_MS_IN_UNIT = 125000; // 0.125ms in ns
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000000);
#else
const s64 EIGTH_OF_A_MS_IN_UNIT = 125; // 0.125ms in us
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000);
#endif

static inline AccelResult accelerate(int x, int y) {
  dbg("FIXEDPT_BITS = %d", FIXEDPT_BITS);

  static ktime_t last_time;
  static s64 last_unit_time = EIGTH_OF_A_MS_IN_UNIT;
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

  if (unit_time < EIGTH_OF_A_MS_IN_UNIT) { // setting a floor for safety
    unit_time = last_unit_time;
  }

  last_unit_time = unit_time;

  fixedpt _unit_time = fixedpt_fromint(unit_time);
  fixedpt millisecond = fixedpt_div(_unit_time, UNIT_PER_MS);

#if FIXEDPT_BITS == 64
  dbg("ktime interval, converting to ns: %lluns -> %sms", unit_time,
      fptoa(millisecond));
#else
  dbg("ktime interval, converting to us: %lluus -> %sms", unit_time,
      fptoa(millisecond));
#endif

  if (millisecond > HUNDRED_MS) { // rounding down to 100 ms
    millisecond = HUNDRED_MS;
  }

  return f_accelerate(x, y, millisecond, atofp(PARAM_SENS_MULT),
                      atofp(PARAM_ACCEL), atofp(PARAM_OFFSET),
                      atofp(PARAM_OUTPUT_CAP));
}

#endif // !_ACCELK_H_
