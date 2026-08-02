#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "accel/linear.h"
#include "accel/mode.h"
#include "accel/natural.h"
#include "accel/synchronous.h"
#include "dbg.h"
#include "fixedptc.h"
#include "math.h"
#include "speed.h"

struct no_accel_curve_args {};

union __accel_args {
  struct natural_curve_args natural;
  struct linear_curve_args linear;
  struct synchronous_curve_args synchronous;
  struct no_accel_curve_args no_accel;
};

struct accel_args {
  fpt sens_mult;
  fpt yx_ratio;
  fpt input_dpi;
  fpt angle_rotation_deg;

  enum accel_mode tag;
  union __accel_args args;
};

union __prepared_accel_args {
  struct prepared_natural_curve_args natural;
  struct prepared_linear_curve_args linear;
  struct prepared_synchronous_curve_args synchronous;
  struct no_accel_curve_args no_accel;
};

struct prepared_accel_args {
  fpt sens_mult;
  fpt yx_ratio;
  fpt dpi_factor;
  fpt cos_angle;
  fpt sin_angle;
  bool rotation_enabled;
  enum accel_mode tag;
  union __prepared_accel_args args;
};

const fpt NORMALIZED_DPI = fpt_fromint(1000);
const fpt DEG_TO_RAD_FACTOR = fpt_xdiv(FIXEDPT_PI, fpt_rconst(180));

static inline struct prepared_accel_args
prepare_accel_args(struct accel_args args) {
  struct prepared_accel_args prepared = {
      .sens_mult = args.sens_mult,
      .yx_ratio = args.yx_ratio,
      .dpi_factor = fpt_div(NORMALIZED_DPI, args.input_dpi),
      .cos_angle = FIXEDPT_ONE,
      .rotation_enabled = args.angle_rotation_deg != 0,
      .tag = args.tag,
  };

  if (prepared.rotation_enabled) {
    fpt radians = fpt_mul(args.angle_rotation_deg, DEG_TO_RAD_FACTOR);
    prepared.cos_angle = fpt_cos(radians);
    prepared.sin_angle = fpt_sin(radians);
  }

  switch (args.tag) {
  case synchronous:
    prepared.args.synchronous =
        prepare_synchronous_curve_args(args.args.synchronous);
    break;
  case natural:
    prepared.args.natural = prepare_natural_curve_args(args.args.natural);
    break;
  case linear:
    prepared.args.linear = prepare_linear_curve_args(args.args.linear);
    break;
  case no_accel:
  default:
    break;
  }

  return prepared;
}

/**
 * Calculate the factor by which to multiply the input vector
 * in order to get the desired output speed.
 *
 */
static inline struct vector sensitivity(fpt input_speed,
                                        struct accel_args args) {
  fpt sens;

  switch (args.tag) {
  case synchronous:
    dbg("accel mode %d: synchronous", args.tag);
    sens = __synchronous_sens_fun(input_speed, args.args.synchronous);
    break;
  case natural:
    dbg("accel mode %d: natural", args.tag);
    sens = __natural_sens_fun(input_speed, args.args.natural);
    break;
  case linear:
    dbg("accel mode %d: linear", args.tag);
    sens = __linear_sens_fun(input_speed, args.args.linear);
    break;
  case no_accel:
    dbg("accel mode %d: no_accel", args.tag);
    sens = FIXEDPT_ONE;
    break;
  default:
    sens = FIXEDPT_ONE;
  }

  sens = fpt_mul(sens, args.sens_mult);
  return (struct vector){sens, fpt_mul(sens, args.yx_ratio)};
}

static inline struct vector
prepared_sensitivity(fpt input_speed, struct prepared_accel_args args) {
  fpt sens;

  switch (args.tag) {
  case synchronous:
    dbg("accel mode %d: synchronous", args.tag);
    sens = __prepared_synchronous_sens_fun(input_speed, args.args.synchronous);
    break;
  case natural:
    dbg("accel mode %d: natural", args.tag);
    sens = __prepared_natural_sens_fun(input_speed, args.args.natural);
    break;
  case linear:
    dbg("accel mode %d: linear", args.tag);
    sens = __prepared_linear_sens_fun(input_speed, args.args.linear);
    break;
  case no_accel:
    dbg("accel mode %d: no_accel", args.tag);
    sens = FIXEDPT_ONE;
    break;
  default:
    sens = FIXEDPT_ONE;
  }
  sens = fpt_mul(sens, args.sens_mult);
  return (struct vector){sens, fpt_mul(sens, args.yx_ratio)};
}

static inline void f_accelerate_prepared(int *x, int *y, fpt time_interval_ms,
                                         struct prepared_accel_args args) {
  static fpt carry_x = 0;
  static fpt carry_y = 0;

  fpt dx = fpt_fromint(*x);
  fpt dy = fpt_fromint(*y);

  {
    if (!args.rotation_enabled) {
      goto accel_routine;
    }

    // Rotate input vector
    fpt dx_rot = fpt_mul(dx, args.cos_angle) - fpt_mul(dy, args.sin_angle);
    fpt dy_rot = fpt_mul(dx, args.sin_angle) + fpt_mul(dy, args.cos_angle);

    dbg("rotated x:               %s", fptoa(dx_rot));
    dbg("rotated y:               %s", fptoa(dy_rot));

    dx = dx_rot;
    dy = dy_rot;
  }
accel_routine:

  dbg("in                        (%d, %d)", *x, *y);
  dbg("in: x (fpt conversion) %s", fptoa(dx));
  dbg("in: y (fpt conversion) %s", fptoa(dy));

  dbg("dpi adjustment factor:     %s", fptoa(args.dpi_factor));
  dx = fpt_mul(dx, args.dpi_factor);
  dy = fpt_mul(dy, args.dpi_factor);

  fpt speed_in = input_speed(dx, dy, time_interval_ms);
  struct vector sens = prepared_sensitivity(speed_in, args);
  dbg("scale x                    %s", fptoa(sens.x));
  dbg("scale y                    %s", fptoa(sens.y));

  fpt dx_out = fpt_mul(dx, sens.x);
  fpt dy_out = fpt_mul(dy, sens.y);

  dx_out = fpt_add(dx_out, carry_x);
  dy_out = fpt_add(dy_out, carry_y);

  dbg("out: x                     %s", fptoa(dx_out));
  dbg("out: y                     %s", fptoa(dy_out));

  *x = fpt_toint(dx_out);
  *y = fpt_toint(dy_out);

  dbg("out (int conversion)      (%d, %d)", *x, *y);

  carry_x = fpt_sub(dx_out, fpt_fromint(*x));
  carry_y = fpt_sub(dy_out, fpt_fromint(*y));

  dbg("carry                     (%s, %s)", fptoa(carry_x), fptoa(carry_x));
}

static inline void f_accelerate(int *x, int *y, fpt time_interval_ms,
                                struct accel_args args) {
  f_accelerate_prepared(x, y, time_interval_ms, prepare_accel_args(args));
}

#endif
