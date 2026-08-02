#ifndef __ACCEL_SYNCHRONOUS_H_
#define __ACCEL_SYNCHRONOUS_H_

#include "../fixedptc.h"

struct synchronous_curve_args {
  fpt gamma;
  fpt smooth;
  fpt motivity;
  fpt sync_speed;
};

struct prepared_synchronous_curve_args {
  fpt gamma_const;
  fpt log_motivity;
  fpt log_syncspeed;
  fpt sync_speed;
  fpt sharpness;
  fpt sharpness_recip;
  fpt minimum_sens;
  fpt maximum_sens;
  bool use_linear_clamp;
};

static inline struct prepared_synchronous_curve_args
prepare_synchronous_curve_args(struct synchronous_curve_args args) {
  fpt log_motivity = fpt_ln(args.motivity);
  fpt sharpness = args.smooth == 0 ? fpt_rconst(16.0)
                                   : fpt_div(fpt_rconst(0.5), args.smooth);

  return (struct prepared_synchronous_curve_args){
      .gamma_const = fpt_div(args.gamma, log_motivity),
      .log_motivity = log_motivity,
      .log_syncspeed = fpt_ln(args.sync_speed),
      .sync_speed = args.sync_speed,
      .sharpness = sharpness,
      .sharpness_recip = fpt_div(FIXEDPT_ONE, sharpness),
      .minimum_sens = fpt_div(FIXEDPT_ONE, args.motivity),
      .maximum_sens = args.motivity,
      .use_linear_clamp = sharpness >= fpt_rconst(16.0),
  };
}

/**
 * Sensitivity Function for `Synchronous` Acceleration
 */
static inline fpt
__prepared_synchronous_sens_fun(fpt input_speed,
                                struct prepared_synchronous_curve_args args) {
  // if sharpness >= 16, use linear clamp for activation function.
  // linear clamp means: fpt_clamp(input_speed, -1, 1).
  if (args.use_linear_clamp) {
    fpt log_space =
        fpt_mul(args.gamma_const, (fpt_ln(input_speed) - args.log_syncspeed));

    if (log_space < -FIXEDPT_ONE) {
      return args.minimum_sens;
    }

    if (log_space > FIXEDPT_ONE) {
      return args.maximum_sens;
    }

    return fpt_exp(fpt_mul(log_space, args.log_motivity));
  }

  if (input_speed == args.sync_speed) {
    return FIXEDPT_ONE;
  }

  fpt log_x = fpt_ln(input_speed);
  fpt log_diff = log_x - args.log_syncspeed;

  if (log_diff > 0) {
    fpt log_space = fpt_mul(args.gamma_const, log_diff);
    fpt exponent = fpt_pow(fpt_tanh(fpt_pow(log_space, args.sharpness)),
                           args.sharpness_recip);
    return fpt_exp(fpt_mul(exponent, args.log_motivity));
  } else {
    fpt log_space = fpt_mul(-args.gamma_const, log_diff);
    fpt exponent = -fpt_pow(fpt_tanh(fpt_pow(log_space, args.sharpness)),
                            args.sharpness_recip);
    return fpt_exp(fpt_mul(exponent, args.log_motivity));
  }
}

static inline fpt __synchronous_sens_fun(fpt input_speed,
                                         struct synchronous_curve_args args) {
  return __prepared_synchronous_sens_fun(input_speed,
                                         prepare_synchronous_curve_args(args));
}
#endif
