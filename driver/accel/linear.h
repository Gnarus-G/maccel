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

struct prepared_linear_curve_args {
  fpt accel;
  fpt offset;
  fpt cap;
  fpt sign;
  bool has_cap;
};

static inline struct prepared_linear_curve_args
prepare_linear_curve_args(struct linear_curve_args args) {
  struct prepared_linear_curve_args prepared = {
      .accel = args.accel,
      .offset = args.offset,
      .sign = FIXEDPT_ONE,
      .has_cap = args.output_cap > 0,
  };

  if (prepared.has_cap) {
    prepared.cap = fpt_sub(args.output_cap, FIXEDPT_ONE);
    if (prepared.cap < 0) {
      prepared.cap = -prepared.cap;
      prepared.sign = -prepared.sign;
    }
  }

  return prepared;
}

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
static inline fpt
__prepared_linear_sens_fun(fpt input_speed,
                           struct prepared_linear_curve_args args) {
  dbg("linear: accel             %s", fptoa(args.accel));
  dbg("linear: offset            %s", fptoa(args.offset));

  if (input_speed <= args.offset) {
    return FIXEDPT_ONE;
  }

  fpt sens = linear_base_fn(input_speed, args.accel, args.offset);
  dbg("linear: base_fn sens       %s", fptoa(args.accel));

  if (args.has_cap)
    sens = minsd(sens, args.cap);

  return fpt_add(FIXEDPT_ONE, fpt_mul(args.sign, sens));
}

static inline fpt __linear_sens_fun(fpt input_speed,
                                    struct linear_curve_args args) {
  return __prepared_linear_sens_fun(input_speed,
                                    prepare_linear_curve_args(args));
}
#endif // !__ACCEL_LINEAR_H_
