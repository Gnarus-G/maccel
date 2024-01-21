#include "fixedptc.h"

#define ACCEL_FACTOR fixedpt_rconst(0.3)
#define OUTPUT_CAP fixedpt_rconst(2)

typedef struct {
  s8 x;
  s8 y;
} AccelResult;

AccelResult inline accelerate(s8 x, s8 y, u32 polling_interval) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = fixedpt_rconst(0);
  static fixedpt carry_y = fixedpt_rconst(0);

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  // printk(KERN_INFO "[MOUSE_MOVE] (%d, %d)", (int)dx, (int)dy);

  fixedpt distance =
      fixedpt_sqrt(fixedpt_add(fixedpt_mul(dx, dx), fixedpt_mul(dy, dy)));

  fixedpt speed_in = fixedpt_div(distance, polling_interval);

  fixedpt speed_factor = fixedpt_add(1, fixedpt_mul((ACCEL_FACTOR), speed_in));

  if (speed_factor > OUTPUT_CAP) {
    speed_factor = OUTPUT_CAP;
  }

  fixedpt dx_out = fixedpt_mul(dx, speed_factor);
  fixedpt dy_out = fixedpt_mul(dy, speed_factor);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  // printk(KERN_INFO "[MOUSE_MOVE_ACCEL] (%d, %d)", result.x, result.y);

  return result;
}
