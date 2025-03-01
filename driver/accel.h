#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "dbg.h"
#include "fixedptc.h"
#include "speed.h"

typedef struct {
  int x;
  int y;
} AccelResult;

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

  dbg("param accel                %s", fptoa(param_accel));

  if (input_speed > 0) {
    sens = fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
  }

  sens = fixedpt_mul(sens, param_sens_mult);

  fixedpt output_cap = fixedpt_mul(param_output_cap, param_sens_mult);

  dbg("param sens                 %s", fptoa(sens));
  dbg("sens limit                 %s", fptoa(output_cap));

  if (param_output_cap != 0 && sens > output_cap) {
    return output_cap;
  }

  return sens;
}

static inline AccelResult
f_accelerate(int x, int y, fixedpt time_interval_ms, fixedpt param_sens_mult,
             fixedpt param_yx_ratio, fixedpt param_accel, fixedpt param_offset,
             fixedpt param_output_cap) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = 0;
  static fixedpt carry_y = 0;

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  dbg("in                        (%d, %d)", x, y);
  dbg("in: x (fixedpt conversion) %s", fptoa(dx));
  dbg("in: y (fixedpt conversion) %s", fptoa(dy));

  fixedpt speed_in = input_speed(dx, dy, time_interval_ms);
  fixedpt sens_x = sensitivity(speed_in, param_sens_mult, param_accel,
                               param_offset, param_output_cap);
  dbg("scale x                    %s", fptoa(sens_x));

  fixedpt sens_y = fixedpt_mul(sens_x, param_yx_ratio);
  dbg("scale y                    %s", fptoa(sens_y));

  fixedpt dx_out = fixedpt_mul(dx, sens_x);
  fixedpt dy_out = fixedpt_mul(dy, sens_y);

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
