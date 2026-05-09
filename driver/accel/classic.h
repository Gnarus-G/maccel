#ifndef __ACCEL_CLASSIC_H_
#define __ACCEL_CLASSIC_H_

#include "../dbg.h"
#include "../fixedptc.h"
#include "../math.h"

struct classic_curve_args {
  fpt accel;
  fpt offset;
  fpt output_cap;
  fpt exponent;
};

static inline fpt classic_power(fpt base, fpt exponent) {
  if (exponent == FIXEDPT_TWO) {
    return fpt_mul(base, base);
  }

  return fpt_pow(base, exponent);
}

static inline fpt classic_base_fn(fpt x, fpt accel, fpt input_offset,
                                  fpt exponent) {
  fpt _x = x - input_offset;
  fpt powered_x = classic_power(_x, exponent);
  return fpt_mul(accel, fpt_div(powered_x, x));
}

/**
 * Sensitivity Function for Classic Acceleration
 */
static inline fpt __classic_sens_fun(fpt input_speed,
                                     struct classic_curve_args args) {
  dbg("classic: accel             %s", fptoa(args.accel));
  dbg("classic: offset            %s", fptoa(args.offset));
  dbg("classic: output_cap        %s", fptoa(args.output_cap));
  dbg("classic: exponent          %s", fptoa(args.exponent));

  if (input_speed <= args.offset || args.exponent <= 0) {
    return FIXEDPT_ONE;
  }

  fpt sens =
      classic_base_fn(input_speed, args.accel, args.offset, args.exponent);
  dbg("classic: base_fn sens      %s", fptoa(sens));

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

#endif // !__ACCEL_CLASSIC_H_
