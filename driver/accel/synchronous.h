#ifndef __ACCEL_SYNCHRONOUS_H_
#define __ACCEL_SYNCHRONOUS_H_

#include "../fixedptc.h"

struct synchronous_curve_args {
  fpt gamma;
  fpt smooth;
  fpt motivity;
  fpt sync_speed;
};

/**
 * Sensitivity Function for `Synchronous` Acceleration
 */
static inline fpt __synchronous_sens_fun(fpt input_speed,
                                         struct synchronous_curve_args args) {
  fpt log_motivity = fpt_ln(args.motivity);
  fpt gamma_const = fpt_div(args.gamma, log_motivity);
  fpt log_syncspeed = fpt_ln(args.sync_speed);
  fpt syncspeed = args.sync_speed;
  fpt sharpness = args.smooth == 0.0 ? fpt_rconst(16.0)
                                     : fpt_div(fpt_rconst(0.5), args.smooth);
  int use_linear_clamp = sharpness >= fpt_rconst(16.0);
  fpt sharpness_recip = fpt_div(FIXEDPT_ONE, sharpness);
  fpt minimum_sens = fpt_div(FIXEDPT_ONE, args.motivity);
  fpt maximum_sens = args.motivity;

  // if sharpness >= 16, use linear clamp for activation function.
  // linear clamp means: fpt_clamp(input_speed, -1, 1).
  if (use_linear_clamp) {
    fpt log_space = fpt_mul(gamma_const, (fpt_ln(input_speed) - log_syncspeed));

    if (log_space < -FIXEDPT_ONE) {
      return minimum_sens;
    }

    if (log_space > FIXEDPT_ONE) {
      return maximum_sens;
    }

    return fpt_exp(fpt_mul(log_space, log_motivity));
  }

  if (input_speed == syncspeed) {
    return FIXEDPT_ONE;
  }

  fpt log_x = fpt_ln(input_speed);
  fpt log_diff = log_x - log_syncspeed;

  if (log_diff > 0) {
    fpt log_space = fpt_mul(gamma_const, log_diff);
    fpt exponent =
        fpt_pow(fpt_tanh(fpt_pow(log_space, sharpness)), sharpness_recip);
    return fpt_exp(fpt_mul(exponent, log_motivity));
  } else {
    fpt log_space = fpt_mul(-gamma_const, log_diff);
    fpt exponent =
        -fpt_pow(fpt_tanh(fpt_pow(log_space, sharpness)), sharpness_recip);
    return fpt_exp(fpt_mul(exponent, log_motivity));
  }
}
#endif
