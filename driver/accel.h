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
extern inline fixedpt sensitivity(fixedpt input_speed, fixedpt param_sens_mult,
                                  fixedpt param_accel, fixedpt param_offset,
                                  fixedpt param_output_cap) {

  input_speed = fixedpt_sub(input_speed, param_offset);

  fixedpt sens = FIXEDPT_ONE;

  dbg("accel    %s", fixedpt_cstr(param_accel, 6));

  if (input_speed > FIXEDPT_ZERO) {
    sens = fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
  }

  sens = fixedpt_mul(sens, param_sens_mult);

  fixedpt output_cap = fixedpt_mul(param_output_cap, param_sens_mult);

  dbg("sens     %s", fixedpt_cstr(sens, 6));
  dbg("sens cap %s", fixedpt_cstr(output_cap, 6));

  if (param_output_cap != FIXEDPT_ZERO && sens > output_cap) {
    return output_cap;
  }

  return sens;
}

static inline fixedpt input_speed(fixedpt dx, fixedpt dy,
                                  u32 polling_interval) {
  fixedpt distance =
      fixedpt_sqrt(fixedpt_add(fixedpt_mul(dx, dx), fixedpt_mul(dy, dy)));

  dbg("distance (in)              %s", fixedpt_cstr(distance, 6));

  fixedpt speed_in = fixedpt_div(distance, fixedpt_fromint(polling_interval));

  dbg("polling interval           %s",
      fixedpt_cstr(fixedpt_fromint(polling_interval), 6));
  dbg("speed (in)                 %s", fixedpt_cstr(speed_in, 6));

  return speed_in;
}

static inline AccelResult f_accelerate(int x, int y, u32 polling_interval,
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
  dbg("in: x (fixedpt conversion) %s", fixedpt_cstr(dx, 6));
  dbg("in: y (fixedpt conversion) %s", fixedpt_cstr(dy, 6));

  fixedpt speed_in = input_speed(dx, dy, polling_interval);
  fixedpt sens = sensitivity(speed_in, param_sens_mult, param_accel,
                             param_offset, param_output_cap);

  dbg("sens %s", fixedpt_cstr(sens, 6));

  fixedpt dx_out = fixedpt_mul(dx, sens);
  fixedpt dy_out = fixedpt_mul(dy, sens);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  dbg("out: x                     %s", fixedpt_cstr(dx_out, 6));
  dbg("out: y                     %s", fixedpt_cstr(dy_out, 6));

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  dbg("out (int conversion)      (%d, %d)", result.x, result.y);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  dbg("carry                     (%s, %s)", fixedpt_cstr(carry_x, 6),
      fixedpt_cstr(carry_x, 6));

  return result;
}

#endif
