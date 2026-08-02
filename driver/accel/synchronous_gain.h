#ifndef __ACCEL_SYNCHRONOUS_GAIN_H_
#define __ACCEL_SYNCHRONOUS_GAIN_H_

#include "../fixedptc.h"

#define SYNCHRONOUS_GAIN_LUT_START -3
#define SYNCHRONOUS_GAIN_LUT_STOP 9
#define SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE 8
#define SYNCHRONOUS_GAIN_LUT_SIZE                                              \
  ((SYNCHRONOUS_GAIN_LUT_STOP - SYNCHRONOUS_GAIN_LUT_START) *                  \
       SYNCHRONOUS_GAIN_LUT_POINTS_PER_OCTAVE +                                \
   1)

struct synchronous_gain_curve_args {
  fpt gamma;
  fpt smooth;
  fpt motivity;
  fpt sync_speed;
};

struct synchronous_gain_lut {
  fpt velocity[SYNCHRONOUS_GAIN_LUT_SIZE];
};

static inline fpt __synchronous_gain_ln(fpt value) {
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

static inline fpt __synchronous_gain_tanh(fpt value) {
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

static inline fpt
__synchronous_gain_activation(fpt input_speed,
                              struct synchronous_gain_curve_args args) {
  fpt log_motivity = __synchronous_gain_ln(args.motivity);
  fpt gamma_const = fpt_div(args.gamma, log_motivity);
  fpt log_syncspeed = __synchronous_gain_ln(args.sync_speed);
  fpt sharpness = args.smooth == 0 ? fpt_rconst(16.0)
                                   : fpt_div(fpt_rconst(0.5), args.smooth);
  int use_linear_clamp = sharpness >= fpt_rconst(16.0);
  fpt sharpness_recip = fpt_div(FIXEDPT_ONE, sharpness);

  if (use_linear_clamp) {
    fpt log_space = fpt_mul(
        gamma_const, (__synchronous_gain_ln(input_speed) - log_syncspeed));

    if (log_space < -FIXEDPT_ONE)
      return fpt_div(FIXEDPT_ONE, args.motivity);
    if (log_space > FIXEDPT_ONE)
      return args.motivity;
    return fpt_exp(fpt_mul(log_space, log_motivity));
  }

  if (input_speed == args.sync_speed)
    return FIXEDPT_ONE;

  {
    fpt log_diff = __synchronous_gain_ln(input_speed) - log_syncspeed;

    if (log_diff > 0) {
      fpt log_space = fpt_mul(gamma_const, log_diff);
      fpt exponent =
          fpt_pow(__synchronous_gain_tanh(fpt_pow(log_space, sharpness)),
                  sharpness_recip);
      return fpt_exp(fpt_mul(exponent, log_motivity));
    } else {
      fpt log_space = fpt_mul(-gamma_const, log_diff);
      fpt exponent =
          -fpt_pow(__synchronous_gain_tanh(fpt_pow(log_space, sharpness)),
                   sharpness_recip);
      return fpt_exp(fpt_mul(exponent, log_motivity));
    }
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
                            struct synchronous_gain_curve_args args) {
  fpt sum = 0;
  fpt previous_x = 0;
  int i;

  for (i = 0; i < SYNCHRONOUS_GAIN_LUT_SIZE; i++) {
    fpt x = __synchronous_gain_x(i);
    fpt interval = fpt_div(x - previous_x, fpt_fromint(2));

    sum += fpt_mul(__synchronous_gain_activation(previous_x + interval, args),
                   interval);
    sum += fpt_mul(__synchronous_gain_activation(x, args), interval);
    lut->velocity[i] = sum;
    previous_x = x;
  }
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

  velocity = lut->velocity[index] +
             fpt_mul(index_f - fpt_fromint(index),
                     lut->velocity[index + 1] - lut->velocity[index]);
  return fpt_div(velocity, input_speed);
}

#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/spinlock.h>

static DEFINE_RWLOCK(synchronous_gain_lut_lock);
static DEFINE_MUTEX(synchronous_gain_lut_update_lock);
static struct synchronous_gain_lut synchronous_gain_lut;

static inline void
update_synchronous_gain_lut(struct synchronous_gain_curve_args args) {
  static struct synchronous_gain_lut next_lut;

  mutex_lock(&synchronous_gain_lut_update_lock);
  __synchronous_gain_lut_init(&next_lut, args);
  write_lock(&synchronous_gain_lut_lock);
  synchronous_gain_lut = next_lut;
  write_unlock(&synchronous_gain_lut_lock);
  mutex_unlock(&synchronous_gain_lut_update_lock);
}

static inline fpt
synchronous_gain_sensitivity(fpt input_speed,
                             struct synchronous_gain_curve_args args) {
  fpt sens;

  read_lock(&synchronous_gain_lut_lock);
  sens = __synchronous_gain_lut_lookup(input_speed, &synchronous_gain_lut);
  read_unlock(&synchronous_gain_lut_lock);
  return sens;
}
#else
static inline fpt
synchronous_gain_sensitivity(fpt input_speed,
                             struct synchronous_gain_curve_args args) {
  static _Thread_local struct synchronous_gain_lut lut;
  static _Thread_local struct synchronous_gain_curve_args cached_args;
  static _Thread_local int initialized;

  if (!initialized || args.gamma != cached_args.gamma ||
      args.smooth != cached_args.smooth ||
      args.motivity != cached_args.motivity ||
      args.sync_speed != cached_args.sync_speed) {
    __synchronous_gain_lut_init(&lut, args);
    cached_args = args;
    initialized = 1;
  }

  return __synchronous_gain_lut_lookup(input_speed, &lut);
}
#endif

#endif
