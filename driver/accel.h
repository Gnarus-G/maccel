#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "accel/linear.h"
#include "accel/mode.h"
#include "accel/natural.h"
#include "dbg.h"
#include "fixedptc.h"
#include "math.h"
#include "speed.h"

/* #include "accel/linear.h" */
union __accel_args {
  struct natural_curve_args natural;
  struct linear_curve_args linear;
};

struct accel_args {
  fixedpt sens_mult;
  fixedpt yx_ratio;
  fixedpt input_dpi;

  enum accel_mode tag;
  union __accel_args args;
};

const fixedpt NORMALIZED_DPI = fixedpt_fromint(1000);

/**
 * Calculate the factor by which to multiply the input vector
 * in order to get the desired output speed.
 *
 */
static inline struct vector sensitivity(fixedpt input_speed,
                                        struct accel_args args) {
  fixedpt sens;

  switch (args.tag) {
  case natural:
    dbg("accel mode %d: natural", args.tag);
    sens = __natural_sens_fun(input_speed, args.args.natural);
    break;
  case linear:
  default:
    dbg("accel mode %d: linear", args.tag);
    sens = __linear_sens_fun(input_speed, args.args.linear);
  }
  sens = fixedpt_mul(sens, args.sens_mult);
  return (struct vector){sens, fixedpt_mul(sens, args.yx_ratio)};
}

static inline void f_accelerate(int *x, int *y, fixedpt time_interval_ms,
                                struct accel_args args) {
  /* AccelResult result = {.x = 0, .y = 0}; */

  static fixedpt carry_x = 0;
  static fixedpt carry_y = 0;

  fixedpt dx = fixedpt_fromint(*x);
  fixedpt dy = fixedpt_fromint(*y);

  dbg("in                        (%d, %d)", *x, *y);
  dbg("in: x (fixedpt conversion) %s", fptoa(dx));
  dbg("in: y (fixedpt conversion) %s", fptoa(dy));

  fixedpt speed_in = input_speed(dx, dy, time_interval_ms);
  struct vector sens = sensitivity(speed_in, args);
  dbg("scale x                    %s", fptoa(sens.x));
  dbg("scale y                    %s", fptoa(sens.y));

  fixedpt dpi_factor = fixedpt_div(NORMALIZED_DPI, args.input_dpi);
  dbg("dpi adjustment factor:     %s", fptoa(dpi_factor));
  sens.x = fixedpt_mul(sens.x, dpi_factor);
  sens.y = fixedpt_mul(sens.y, dpi_factor);

  fixedpt dx_out = fixedpt_mul(dx, sens.x);
  fixedpt dy_out = fixedpt_mul(dy, sens.y);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  dbg("out: x                     %s", fptoa(dx_out));
  dbg("out: y                     %s", fptoa(dy_out));

  *x = fixedpt_toint(dx_out);
  *y = fixedpt_toint(dy_out);

  dbg("out (int conversion)      (%d, %d)", *x, *y);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(*x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(*y));

  dbg("carry                     (%s, %s)", fptoa(carry_x), fptoa(carry_x));
}

#endif
