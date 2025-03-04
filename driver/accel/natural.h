#ifndef __ACCEL_NATURAL_H_
#define __ACCEL_NATURAL_H_

#include "../fixedptc.h"

struct natural_curve_args {
  fixedpt decay_rate;
  fixedpt offset;
  fixedpt limit;
};

/**
 * Gain Function for Natural Acceleration
 */
static inline fixedpt __natural_sens_fun(fixedpt input_speed,
                                         struct natural_curve_args args) {
  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  if (args.limit <= FIXEDPT_ONE) {
    return FIXEDPT_ONE;
  }

  if (args.decay_rate <= 0) {
    return FIXEDPT_ONE;
  }

  fixedpt limit = args.limit - FIXEDPT_ONE;
  fixedpt accel = fixedpt_div(args.decay_rate, fixedpt_abs(limit));
  fixedpt constant = fixedpt_div(-limit, accel);

  dbg("natural: constant              %s", fptoa(constant));

  fixedpt offset_x = args.offset - input_speed;
  dbg("natural: offset              %s", fptoa(offset_x));
  fixedpt decay = fixedpt_exp(fixedpt_mul(accel, offset_x));

  dbg("natural: decay              %s", fptoa(decay));

  fixedpt output_denom = fixedpt_div(decay, accel) - offset_x;
  fixedpt output = fixedpt_mul(limit, output_denom) + constant;

  return fixedpt_div(output, input_speed) + FIXEDPT_ONE;
}
#endif
