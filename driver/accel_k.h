#ifndef _ACCELK_H_
#define _ACCELK_H_

#include "accel.h"
#include "accel/linear.h"
#include "fixedptc.h"
#include "linux/ktime.h"
#include "params.h"

static struct accel_args collect_args(void) {
  enum accel_mode mode = PARAM_MODE;
  struct accel_args args = {0};
  args.tag = mode;

  args.param_sens_mult = atofp(PARAM_SENS_MULT);
  args.param_yx_ratio = atofp(PARAM_YX_RATIO);

  switch (mode) {
  case natural: {
    struct natural_curve_args natural;
    natural.decay_rate = atofp(PARAM_DECAY_RATE);
    natural.offset = atofp(PARAM_OFFSET);
    natural.limit = atofp(PARAM_LIMIT);
    break;
  }
  case linear:
  default: {
    struct linear_curve_args linear;
    linear.accel = atofp(PARAM_ACCEL);
    linear.offset = atofp(PARAM_OFFSET);
    linear.output_cap = atofp(PARAM_OUTPUT_CAP);
  }
  };
  return args;
}

#if FIXEDPT_BITS == 64
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000000); // 1 million nanoseconds
#else
const fixedpt UNIT_PER_MS = fixedpt_rconst(1000); // 1 thousand microsends
#endif

static inline void accelerate(int *x, int *y) {
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

  return f_accelerate(x, y, millisecond, collect_args());
}

#endif // !_ACCELK_H_
