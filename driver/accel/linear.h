#ifndef __ACCEL_LINEAR_H_
#define __ACCEL_LINEAR_H_

#include "../dbg.h"
#include "../fixedptc.h"
#include "../math.h"

struct linear_curve_args {
  fpt accel;
  fpt offset;
  fpt output_cap;
};

static inline fpt linear_base_fn(fpt x, fpt accel,
                                     fpt input_offset) {
  fpt _x = x - input_offset;
  fpt _x_square = fpt_mul(
      _x, _x); // because linear in rawaccel is classic with exponent = 2
  return fpt_mul(accel, fpt_div(_x_square, x));
}

/**
 * Sensitivity Function for Linear Acceleration
 */
static inline fpt __linear_sens_fun(fpt input_speed,
                                        struct linear_curve_args args) {
  dbg("linear: accel             %s", fptoa(args.accel));
  dbg("linear: offset            %s", fptoa(args.offset));
  dbg("linear: output_cap        %s", fptoa(args.output_cap));

  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  fpt sens = linear_base_fn(input_speed, args.accel, args.offset);
  dbg("linear: base_fn sens       %s", fptoa(args.accel));

  fpt sign = FIXEDPT_ONE;
  if (args.output_cap > 0) {
    fpt cap = fpt_sub(args.output_cap, FIXEDPT_ONE);
    if (cap < 0) {
      cap = -cap;
      sign = -sign;
    }
    sens = minsd(sens, cap);
  }

  return fpt_add(FIXEDPT_ONE, fpt_mul(sign, sens));
}
#endif // !__ACCEL_LINEAR_H_
