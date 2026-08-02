#ifndef __ACCEL_SYNCHRONOUS_H_
#define __ACCEL_SYNCHRONOUS_H_

#include "../fixedptc.h"

#define SYNCHRONOUS_GAIN_LUT_START -3
#define SYNCHRONOUS_GAIN_LUT_STOP 9
#define SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE 8
#define SYNCHRONOUS_GAIN_LUT_SIZE                                              \
  ((SYNCHRONOUS_GAIN_LUT_STOP - SYNCHRONOUS_GAIN_LUT_START) *                  \
       SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE +                                \
   1)

struct synchronous_curve_args {
  fpt gamma;
  fpt smooth;
  fpt motivity;
  fpt sync_speed;
  fpt gain;
};

struct synchronous_gain_lut {
  fpt velocity[SYNCHRONOUS_GAIN_LUT_SIZE];
};

static inline fpt __synchronous_ln(fpt value) {
  const fpt ln_two = fpt_rconst(0.69314718055994530942);
  int exponent = 0;

  if (value <= 0)
    return fpt_ln(value);
  while (value < FIXEDPT_ONE) {
    value <<= 1;
    exponent--;
  }
  return fpt_ln(value) + fpt_mul(fpt_fromint(exponent), ln_two);
}

static inline fpt __synchronous_tanh(fpt value) {
#if FIXEDPT_BITS == 64
  const fpt saturation = fpt_fromint(10);
#else
  const fpt saturation = fpt_fromint(5);
#endif

  if (value >= saturation)
    return FIXEDPT_ONE;
  if (value <= -saturation)
    return -FIXEDPT_ONE;
  return fpt_tanh(value);
}

/**
 * Sensitivity Function for `Synchronous` Acceleration
 */
static inline fpt __synchronous_sens_fun(fpt input_speed,
                                         struct synchronous_curve_args args) {
  fpt log_motivity = __synchronous_ln(args.motivity);
  fpt gamma_const = fpt_div(args.gamma, log_motivity);
  fpt log_syncspeed = __synchronous_ln(args.sync_speed);
  fpt syncspeed = args.sync_speed;
  fpt sharpness = args.smooth == 0 ? fpt_rconst(16.0)
                                   : fpt_div(fpt_rconst(0.5), args.smooth);
  int use_linear_clamp = sharpness >= fpt_rconst(16.0);
  fpt sharpness_recip = fpt_div(FIXEDPT_ONE, sharpness);
  fpt minimum_sens = fpt_div(FIXEDPT_ONE, args.motivity);
  fpt maximum_sens = args.motivity;

  // if sharpness >= 16, use linear clamp for activation function.
  // linear clamp means: fpt_clamp(input_speed, -1, 1).
  if (use_linear_clamp) {
    fpt log_space =
        fpt_mul(gamma_const, (__synchronous_ln(input_speed) - log_syncspeed));

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

  fpt log_x = __synchronous_ln(input_speed);
  fpt log_diff = log_x - log_syncspeed;

  if (log_diff > 0) {
    fpt log_space = fpt_mul(gamma_const, log_diff);
    fpt exponent = fpt_pow(__synchronous_tanh(fpt_pow(log_space, sharpness)),
                           sharpness_recip);
    return fpt_exp(fpt_mul(exponent, log_motivity));
  } else {
    fpt log_space = fpt_mul(-gamma_const, log_diff);
    fpt exponent = -fpt_pow(__synchronous_tanh(fpt_pow(log_space, sharpness)),
                            sharpness_recip);
    return fpt_exp(fpt_mul(exponent, log_motivity));
  }
}

static inline fpt __synchronous_gain_pow2(int exponent) {
  if (exponent >= 0)
    return fpt_fromint(1 << exponent);

  return FIXEDPT_ONE >> -exponent;
}

static inline fpt __synchronous_gain_x(int index) {
  int octave = index / SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE;
  int step = index % SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE;
  fpt octave_start =
      __synchronous_gain_pow2(SYNCHRONOUS_GAIN_LUT_START + octave);

  return fpt_mul(fpt_fromint(step + SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE),
                 fpt_div(octave_start,
                         fpt_fromint(SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE)));
}

static inline void
__synchronous_gain_lut_init(struct synchronous_gain_lut *lut,
                            struct synchronous_curve_args args) {
  fpt sum = 0;
  fpt previous_x = 0;
  int i;

  for (i = 0; i < SYNCHRONOUS_GAIN_LUT_SIZE; i++) {
    fpt x = i == SYNCHRONOUS_GAIN_LUT_SIZE - 1
                ? __synchronous_gain_pow2(SYNCHRONOUS_GAIN_LUT_STOP)
                : __synchronous_gain_x(i);
    fpt interval = fpt_div(x - previous_x, fpt_fromint(2));
    int partition;

    for (partition = 1; partition <= 2; partition++) {
      fpt sample_x = previous_x + fpt_mul(fpt_fromint(partition), interval);
      sum += fpt_mul(__synchronous_sens_fun(sample_x, args), interval);
    }

    lut->velocity[i] = sum;
    previous_x = x;
  }
}

static inline fpt __synchronous_gain_lerp(fpt a, fpt b, fpt t) {
  return a + fpt_mul(t, b - a);
}

static inline fpt
__synchronous_gain_lut_lookup(fpt input_speed,
                              const struct synchronous_gain_lut *lut) {
  const fpt x_start = __synchronous_gain_pow2(SYNCHRONOUS_GAIN_LUT_START);
  int exponent = SYNCHRONOUS_GAIN_LUT_START;
  fpt base;
  fpt index_f;
  int index;
  fpt velocity;

  if (input_speed <= x_start)
    return fpt_div(lut->velocity[0], x_start);

  while (exponent < SYNCHRONOUS_GAIN_LUT_STOP - 1 &&
         input_speed >= __synchronous_gain_pow2(exponent + 1))
    exponent++;

  base = __synchronous_gain_pow2(exponent);
  index_f = fpt_fromint((exponent - SYNCHRONOUS_GAIN_LUT_START) *
                        SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE) +
            fpt_mul(fpt_div(input_speed - base, base),
                    fpt_fromint(SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE));
  index = fpt_toint(index_f);
  if (index > SYNCHRONOUS_GAIN_LUT_SIZE - 2)
    index = SYNCHRONOUS_GAIN_LUT_SIZE - 2;

  velocity =
      __synchronous_gain_lerp(lut->velocity[index], lut->velocity[index + 1],
                              index_f - fpt_fromint(index));
  return fpt_div(velocity, input_speed);
}
#endif
