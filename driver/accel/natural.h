#ifndef __ACCEL_NATURAL_H_
#define __ACCEL_NATURAL_H_

#include "../fixedptc.h"

struct natural_curve_args {
  fpt decay_rate;
  fpt offset;
  fpt limit;
};

struct prepared_natural_curve_args {
  fpt offset;
  fpt limit;
  fpt accel;
  fpt constant;
  bool enabled;
};

static inline struct prepared_natural_curve_args
prepare_natural_curve_args(struct natural_curve_args args) {
  struct prepared_natural_curve_args prepared = {
      .offset = args.offset,
      .enabled = args.limit > FIXEDPT_ONE && args.decay_rate > 0,
  };

  if (prepared.enabled) {
    prepared.limit = args.limit - FIXEDPT_ONE;
    prepared.accel = fpt_div(args.decay_rate, fpt_abs(prepared.limit));
    prepared.constant = fpt_div(-prepared.limit, prepared.accel);
  }

  return prepared;
}

/**
 * Gain Function for Natural Acceleration
 */
static inline fpt
__prepared_natural_sens_fun(fpt input_speed,
                            struct prepared_natural_curve_args args) {
  dbg("natural: offset            %s", fptoa(args.offset));
  dbg("natural: limit             %s", fptoa(args.limit));
  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  if (!args.enabled) {
    return FIXEDPT_ONE;
  }

  dbg("natural: constant          %s", fptoa(args.constant));

  fpt offset_x = args.offset - input_speed;
  fpt decay = fpt_exp(fpt_mul(args.accel, offset_x));

  dbg("natural: decay             %s", fptoa(decay));

  fpt output_denom = fpt_div(decay, args.accel) - offset_x;
  fpt output = fpt_mul(args.limit, output_denom) + args.constant;

  return fpt_div(output, input_speed) + FIXEDPT_ONE;
}

static inline fpt __natural_sens_fun(fpt input_speed,
                                     struct natural_curve_args args) {
  return __prepared_natural_sens_fun(input_speed,
                                     prepare_natural_curve_args(args));
}
#endif
