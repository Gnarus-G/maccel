#ifndef __ACCEL_NO_ACCEL_H
#define __ACCEL_NO_ACCEL_H

#include "../fixedptc.h"

// No specific arguments for no_accel mode
struct no_accel_curve_args {};

static inline fpt __no_accel_sens_fun(fpt input_speed,
                                      struct no_accel_curve_args args) {
  (void)input_speed; // Unused
  (void)args;        // Unused
  return FIXEDPT_ONE;
}

#endif // !__ACCEL_NO_ACCEL_H
