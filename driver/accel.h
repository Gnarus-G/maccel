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
  fpt angle_snap_threshold;

  enum accel_mode tag;
  union __accel_args args;
};

const fpt NORMALIZED_DPI = fpt_fromint(1000);

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

const fpt DEG_TO_RAD_FACTOR = fpt_xdiv(FIXEDPT_PI, fpt_rconst(180));

static inline fpt snap_threshold(fpt degrees) {
  return fpt_tan(fpt_mul(degrees, DEG_TO_RAD_FACTOR));
}

static inline struct vector snap_to_axis(struct vector input, fpt threshold) {
  fpt abs_x = fpt_abs(input.x);
  fpt abs_y = fpt_abs(input.y);

  if (abs_x < fpt_mul(abs_y, threshold)) {
    fpt length = scaled_magnitude(input);
    return (struct vector){0, input.y < 0 ? -length : length};
  }

  if (abs_y < fpt_mul(abs_x, threshold)) {
    fpt length = scaled_magnitude(input);
    return (struct vector){input.x < 0 ? -length : length, 0};
  }

  return input;
}

static inline void f_accelerate(int *x, int *y, fpt time_interval_ms,
                                struct accel_args args) {
  static fpt carry_x = 0;
  static fpt carry_y = 0;

  fpt dx = fpt_fromint(*x);
  fpt dy = fpt_fromint(*y);

  if (args.angle_rotation_deg != 0) {
    // Add rotation of input vector
    fpt degrees = args.angle_rotation_deg;
    fpt radians =
        fpt_mul(degrees, DEG_TO_RAD_FACTOR); // Convert degrees to radians

    fpt cos_angle = fpt_cos(radians);
    fpt sin_angle = fpt_sin(radians);

    dbg("rotation angle(deg):     %s deg", fptoa(degrees));
    dbg("rotation angle(rad):     %s rad", fptoa(radians));
    dbg("cosine of rotation:      %s", fptoa(cos_angle));
    dbg("sine of rotation:        %s", fptoa(sin_angle));

    // Rotate input vector
    fpt dx_rot = fpt_mul(dx, cos_angle) - fpt_mul(dy, sin_angle);
    fpt dy_rot = fpt_mul(dx, sin_angle) + fpt_mul(dy, cos_angle);

    dbg("rotated x:               %s", fptoa(dx_rot));
    dbg("rotated y:               %s", fptoa(dy_rot));

    dx = dx_rot;
    dy = dy_rot;
  }

  if (args.angle_snap_threshold > 0) {
    struct vector snapped =
        snap_to_axis((struct vector){dx, dy}, args.angle_snap_threshold);
    dx = snapped.x;
    dy = snapped.y;

    if (dx == 0) {
      carry_x = 0;
    }
    if (dy == 0) {
      carry_y = 0;
    }
  }

  dbg("in                        (%d, %d)", *x, *y);
  dbg("in: x (fpt conversion) %s", fptoa(dx));
  dbg("in: y (fpt conversion) %s", fptoa(dy));

  fpt dpi_factor = fpt_div(NORMALIZED_DPI, args.input_dpi);
  dbg("dpi adjustment factor:     %s", fptoa(dpi_factor));
  dx = fpt_mul(dx, dpi_factor);
  dy = fpt_mul(dy, dpi_factor);

  fpt speed_in;
  if (args.angle_snap_threshold > 0 && (dx == 0 || dy == 0)) {
    fpt distance = dx == 0 ? fpt_abs(dy) : fpt_abs(dx);
    speed_in = input_speed_from_distance(distance, time_interval_ms);
  } else {
    speed_in = input_speed(dx, dy, time_interval_ms);
  }
  struct vector sens = sensitivity(speed_in, args);
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

#endif
