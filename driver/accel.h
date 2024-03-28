#include "dbg.h"
#include "fixedptc.h"

typedef signed char s8;
typedef unsigned int u32;

typedef struct {
  s8 x;
  s8 y;
} AccelResult;

static const fixedpt FIXEDPT_ZERO = fixedpt_rconst(0.0);

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

  if (input_speed > FIXEDPT_ZERO) {
    sens = fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
  }

  sens = fixedpt_mul(sens, param_sens_mult);

  fixedpt output_cap = fixedpt_mul(param_output_cap, param_sens_mult);

  if (param_output_cap != FIXEDPT_ZERO && sens > output_cap) {
    sens = output_cap;
  }

  return sens;
}

static inline AccelResult f_accelerate(s8 x, s8 y, u32 polling_interval,
                                       fixedpt param_sens_mult,
                                       fixedpt param_accel,
                                       fixedpt param_offset,
                                       fixedpt param_output_cap) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = FIXEDPT_ZERO;
  static fixedpt carry_y = FIXEDPT_ZERO;

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  dbg("[MOUSE_MOVE] (%s, %s)", fixedpt_cstr(dx, 5), fixedpt_cstr(dy, 5));

  fixedpt distance =
      fixedpt_sqrt(fixedpt_add(fixedpt_mul(dx, dx), fixedpt_mul(dy, dy)));

  dbg("distance %s", fixedpt_cstr(distance, 5));

  fixedpt speed_in = fixedpt_div(distance, fixedpt_fromint(polling_interval));

  dbg("input speed %s, with interval %s", fixedpt_cstr(speed_in, 5),
      fixedpt_cstr(fixedpt_fromint(polling_interval), 5));

  fixedpt sens = sensitivity(speed_in, param_sens_mult, param_accel,
                             param_offset, param_output_cap);

  fixedpt dx_out = fixedpt_mul(dx, sens);
  fixedpt dy_out = fixedpt_mul(dy, sens);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  return result;
}
