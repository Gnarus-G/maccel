#include "fixedptc.h"

typedef signed char s8;
typedef unsigned int u32;

typedef struct {
  s8 x;
  s8 y;
} AccelResult;

/**
 * Calculate the normalized factor by which to multiply the input vector
 * in order to get the desired output speed.
 *
 */
extern inline fixedpt acceleration_factor(fixedpt input_speed,
                                          fixedpt param_accel,
                                          fixedpt param_offset,
                                          fixedpt param_output_cap) {

  // printk("input speed %s, with interval %s", fixedpt_cstr(speed_in, 5),
  //        fixedpt_cstr(polling_interval, 5));

  input_speed = fixedpt_sub(input_speed, param_offset);

  fixedpt accel_factor = FIXEDPT_ONE;

  if (input_speed > fixedpt_rconst(0.0)) {
    accel_factor =
        fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));

    if (accel_factor > param_output_cap) {
      accel_factor = param_output_cap;
    }
  }

  return accel_factor;
}

inline AccelResult f_accelerate(s8 x, s8 y, u32 polling_interval,
                                fixedpt param_accel, fixedpt param_offset,
                                fixedpt param_output_cap) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = fixedpt_rconst(0);
  static fixedpt carry_y = fixedpt_rconst(0);

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  // printk(KERN_INFO "[MOUSE_MOVE] (%s, %s)", fixedpt_cstr(dx, 5),
  //        fixedpt_cstr(dy, 5));

  fixedpt distance =
      fixedpt_sqrt(fixedpt_add(fixedpt_mul(dx, dx), fixedpt_mul(dy, dy)));

  // printk("distance %s", fixedpt_cstr(distance, 5));

  fixedpt speed_in = fixedpt_div(distance, fixedpt_fromint(polling_interval));

  // printk("input speed %s, with interval %s", fixedpt_cstr(speed_in, 5),
  //        fixedpt_cstr(polling_interval, 5));

  fixedpt accel_factor = acceleration_factor(speed_in, param_accel,
                                             param_offset, param_output_cap);

  // printk("accel_factor %s", fixedpt_cstr(accel_factor, 5));

  fixedpt dx_out = fixedpt_mul(dx, accel_factor);
  fixedpt dy_out = fixedpt_mul(dy, accel_factor);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  // printk(KERN_INFO "[MOUSE_MOVE_ACCEL] (%d, %d)", result.x, result.y);

  // printk(KERN_INFO "[MOUSE_MOVE] carry (%s, %s)", fixedpt_cstr(carry_x, 5),
  //        fixedpt_cstr(carry_y, 5));

  return result;
}
