#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "dbg.h"
#include "fixedptc.h"

typedef unsigned int u32;

typedef struct {
  int x;
  int y;
} AccelResult;

const fixedpt FIXEDPT_ZERO = fixedpt_rconst(0.0);

/**
 * Calculate the factor by which to multiply the input vector
 * in order to get the desired output speed.
 *
 */
static inline fixedpt sensitivity(fixedpt input_speed, fixedpt param_sens_mult,
                                  fixedpt param_accel, fixedpt param_offset,
                                  fixedpt param_output_cap) {

  input_speed = fixedpt_sub(input_speed, param_offset);

  fixedpt sens = FIXEDPT_ONE;

  dbg("accel    %s", fptoa(param_accel));

  if (input_speed > FIXEDPT_ZERO) {
    sens = fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
  }

  sens = fixedpt_mul(sens, param_sens_mult);

  fixedpt output_cap = fixedpt_mul(param_output_cap, param_sens_mult);

  dbg("sens     %s", fptoa(sens));
  dbg("sens cap %s", fptoa(output_cap));

  if (param_output_cap != FIXEDPT_ZERO && sens > output_cap) {
    return output_cap;
  }

  return sens;
}

static inline fixedpt input_speed(fixedpt dx, fixedpt dy, fixedpt time_ms) {
  fixedpt a2 = fixedpt_mul(dx, dx);
  fixedpt b2 = fixedpt_mul(dy, dy);
  fixedpt a2_plus_b2 = fixedpt_add(a2, b2);

  dbg("dx^2 (in)                  %s", fptoa(a2));
  dbg("dy^2 (in)                  %s", fptoa(b2));
  dbg("square modulus (in)        %s", fptoa(a2_plus_b2));

  fixedpt distance = fixedpt_sqrt(a2_plus_b2);

  if (distance == -1) {
    dbg("distance calculation failed: t = %s", fptoa(time_ms));
    return 0;
  }

  dbg("distance (in)              %s", fptoa(distance));

  fixedpt speed = fixedpt_div(distance, time_ms);

  dbg("time interval              %s", fptoa(time_ms));
  dbg("speed (in)                 %s", fptoa(speed));

  return speed;
}

static inline AccelResult f_accelerate(int x, int y, fixedpt time_interval_ms,
                                       fixedpt param_sens_mult,
                                       fixedpt param_accel,
                                       fixedpt param_offset,
                                       fixedpt param_output_cap) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = FIXEDPT_ZERO;
  static fixedpt carry_y = FIXEDPT_ZERO;

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  dbg("in                        (%d, %d)", x, y);
  dbg("in: x (fixedpt conversion) %s", fptoa(dx));
  dbg("in: y (fixedpt conversion) %s", fptoa(dy));

  fixedpt speed_in = input_speed(dx, dy, time_interval_ms);
  fixedpt sens = sensitivity(speed_in, param_sens_mult, param_accel,
                             param_offset, param_output_cap);

  dbg("sens %s", fptoa(sens));

  fixedpt dx_out = fixedpt_mul(dx, sens);
  fixedpt dy_out = fixedpt_mul(dy, sens);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  dbg("out: x                     %s", fptoa(dx_out));
  dbg("out: y                     %s", fptoa(dy_out));

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  dbg("out (int conversion)      (%d, %d)", result.x, result.y);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  dbg("carry                     (%s, %s)", fptoa(carry_x), fptoa(carry_x));

  return result;
}

#endif
