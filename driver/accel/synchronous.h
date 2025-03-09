#ifndef __ACCEL_SYNCHRONOUS_H_
#define __ACCEL_SYNCHRONOUS_H_

#include "../fixedptc.h"

struct synchronous_curve_args {
  fpt gamma;
  fpt smooth;
  fpt motivity;
  fpt sync_speed;
};

/**
 * Sensitivity Function for `Synchronous` Acceleration
 */
static inline fpt __synchronous_sens_fun(fpt input_speed,
                                         struct synchronous_curve_args args) {
  return FIXEDPT_ONE;
}
#endif
