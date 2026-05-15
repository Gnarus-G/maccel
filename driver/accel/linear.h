#ifndef __ACCEL_LINEAR_H_
#define __ACCEL_LINEAR_H_

#include "../dbg.h"
#include "../fixedptc.h"
#include "../math.h"

struct linear_curve_args {
  fpt accel;
  fpt offset;
  fpt output_cap;
  fpt classic_exponent;
};

static inline fpt linear_base_fn(fpt x, fpt accel,
                                     fpt input_offset, fpt classic_exponent) {
  fpt _x = x - input_offset;
  fpt _x_power =
      classic_exponent == FIXEDPT_TWO ? fpt_mul(_x, _x)
                                      : fpt_pow(_x, classic_exponent);
  return fpt_mul(accel, fpt_div(_x_power, x));
}

/**
 * Sensitivity Function for Linear Acceleration
 */
static inline fpt __linear_sens_fun(fpt input_speed,
                                        struct linear_curve_args args) {
  dbg("linear: accel             %s", fptoa(args.accel));
  dbg("linear: offset            %s", fptoa(args.offset));
  dbg("linear: output_cap        %s", fptoa(args.output_cap));
  dbg("linear: classic_exponent  %s", fptoa(args.classic_exponent));

  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  fpt classic_exponent =
      args.classic_exponent > 0 ? args.classic_exponent : FIXEDPT_TWO;
  fpt sens =
      linear_base_fn(input_speed, args.accel, args.offset, classic_exponent);
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
