#ifndef __ACCEL_NATURAL_H_
#define __ACCEL_NATURAL_H_

#include "../fixedptc.h"

struct natural_curve_args {
  fpt decay_rate;
  fpt offset;
  fpt limit;
};

/**
 * Gain Function for Natural Acceleration
 */
static inline fpt __natural_sens_fun(fpt input_speed,
                                         struct natural_curve_args args) {
  dbg("natural: decay_rate        %s", fptoa(args.decay_rate));
  dbg("natural: offset            %s", fptoa(args.offset));
  dbg("natural: limit             %s", fptoa(args.limit));
  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  if (args.limit <= FIXEDPT_ONE) {
    return FIXEDPT_ONE;
  }

  if (args.decay_rate <= 0) {
    return FIXEDPT_ONE;
  }

  fpt limit = args.limit - FIXEDPT_ONE;
  fpt accel = fpt_div(args.decay_rate, fpt_abs(limit));
  fpt constant = fpt_div(-limit, accel);

  dbg("natural: constant          %s", fptoa(constant));

  fpt offset_x = args.offset - input_speed;
  fpt decay = fpt_exp(fpt_mul(accel, offset_x));

  dbg("natural: decay             %s", fptoa(decay));

  fpt output_denom = fpt_div(decay, accel) - offset_x;
  fpt output = fpt_mul(limit, output_denom) + constant;

  return fpt_div(output, input_speed) + FIXEDPT_ONE;
}
#endif
