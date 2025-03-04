#ifndef __ACCEL_LINEAR_H_
#define __ACCEL_LINEAR_H_

#include "../dbg.h"
#include "../fixedptc.h"
#include "../math.h"

struct linear_curve_args {
  fixedpt accel;
  fixedpt offset;
  fixedpt output_cap;
};

static inline fixedpt linear_base_fn(fixedpt x, fixedpt accel,
                                     fixedpt input_offset) {
  fixedpt _x = x - input_offset;
  fixedpt _x_square = fixedpt_mul(
      _x, _x); // because linear in rawaccel is classic with exponent = 2
  return fixedpt_mul(accel, fixedpt_div(_x_square, x));
}

/**
 * Sensitivity Function for Linear Acceleration
 */
static inline fixedpt __linear_sens_fun(fixedpt input_speed,
                                        struct linear_curve_args args) {
  dbg("linear: accel             %s", fptoa(args.accel));
  dbg("linear: offset            %s", fptoa(args.offset));
  dbg("linear: output_cap        %s", fptoa(args.output_cap));

  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  fixedpt sens = linear_base_fn(input_speed, args.accel, args.offset);
  dbg("linear: base_fn sens       %s", fptoa(args.accel));

  fixedpt sign = FIXEDPT_ONE;
  if (args.output_cap > 0) {
    fixedpt cap = fixedpt_sub(args.output_cap, FIXEDPT_ONE);
    if (cap < 0) {
      cap = -cap;
      sign = -sign;
    }
    sens = minsd(sens, cap);
  }

  return fixedpt_add(FIXEDPT_ONE, fixedpt_mul(sign, sens));
}
#endif // !__ACCEL_LINEAR_H_
