#ifndef _ACCELK_H_
#define _ACCELK_H_

#include "accel.h"
#include "linux/ktime.h"
#include "params.h"

static AccelResult inline accelerate(int x, int y) {
  static ktime_t last;
  static u64 last_ms = 1;

  ktime_t now = ktime_get();
  u64 ms = ktime_to_ms(now - last);

  last = now;

  if (ms < 1) { // ensure no less than 1ms
    ms = last_ms;
  }

  last_ms = ms;

  if (ms > 100) { // rounding dow to 100 ms
    ms = 100;
  }

  return f_accelerate(x, y, ms, PARAM_SENS_MULT, PARAM_MODE,
  // linear mode
  PARAM_ACCEL,
  // motivity mode
  PARAM_MOTIVITY, PARAM_GAMMA, PARAM_SYNC_SPEED, 
  PARAM_OFFSET,PARAM_OUTPUT_CAP);
}

#endif // !_ACCELK_H_
