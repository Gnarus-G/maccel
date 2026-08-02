#ifndef _PARAM_H_
#define _PARAM_H_

#include "accel/mode.h"
#include "fixedptc.h"
#include "linux/moduleparam.h"
#include "linux/mutex.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default_value, desc)                                      \
  char *PARAM_##param = #default_value;                                        \
  module_param_named(param, PARAM_##param, charp, RW_USER_GROUP);              \
  MODULE_PARM_DESC(param, desc);

static const struct kernel_param_ops synchronous_param_ops;

#define SYNCHRONOUS_PARAM(param, default_value, desc)                          \
  char *PARAM_##param = #default_value;                                        \
  module_param_cb(param, &synchronous_param_ops, &PARAM_##param,               \
                  RW_USER_GROUP);                                              \
  MODULE_PARM_DESC(param, desc);

#if FIXEDPT_BITS == 64
PARAM(
    SENS_MULT, 4294967296, // 1 << 32
    "A factor applied by the sensitivity calculation after ACCEL is applied.");
PARAM(YX_RATIO, 4294967296, // 1 << 32
      "A factor (Y/X) by which the final sensitivity calculated is multiplied "
      "to produce the sensitivity applied to the Y axis.");
PARAM(INPUT_DPI, 4294967296000, // 1000 << 32
      "The DPI of the mouse, used to normalize input to 1000 DPI equivalent "
      "for consistent acceleration across different mice.");
#else
PARAM(SENS_MULT, 65536, // 1 << 16
      "A factor applied the sensitivity calculation after ACCEL is applied.");
PARAM(YX_RATIO, 65536, // 1 << 16
      "A factor (Y/X) by which the final sensitivity calculated is multiplied "
      "to produce the sensitivity applied to the Y axis.");
PARAM(INPUT_DPI, 65536000, // 1000 << 16
      "The DPI of the mouse, used to normalize input to 1000 DPI equivalent "
      "for consistent acceleration across different mice.");
#endif

PARAM(ANGLE_ROTATION, 0,
      "Apply rotation (degrees) to the mouse movement input");
// For Linear Mode

PARAM(ACCEL, 0, "Control the sensitivity calculation.");
PARAM(OFFSET, 0,
      "Input speed threshold (counts/ms) before acceleration begins.");
PARAM(OUTPUT_CAP, 0, "Control the maximum sensitivity.");

// For Natural Mode

#if FIXEDPT_BITS == 64
PARAM(DECAY_RATE, 429496730, // 0.1 << 32
      "Decay rate of the Natural curve.");
PARAM(LIMIT, 6442450944, // 1.5 << 32
      "Limit of the Natural curve.");
#else
PARAM(DECAY_RATE, 6554, // 0.1 << 16
      "Decay rate of the Natural curve");
PARAM(LIMIT, 98304, // 1.5 << 16
      "Limit of the Natural curve");
#endif

// For Synchronous Mode

#if FIXEDPT_BITS == 64
SYNCHRONOUS_PARAM(
    GAMMA, 4294967296, // 1 << 32
    "Control how fast you get from low to fast around the midpoint");
SYNCHRONOUS_PARAM(SMOOTH, 2147483648, // 0.5 << 32
                  "Control the suddeness of the sensitivity increase.");
SYNCHRONOUS_PARAM(
    MOTIVITY, 6442450944, // 1.5 << 32
    "Set the maximum sensitivity while also setting the minimum to "
    "1/MOTIVITY");
SYNCHRONOUS_PARAM(
    SYNC_SPEED, 21474836480, // 5 << 32
    "Set The middle sensitivity between you min and max sensitivity");
#else
SYNCHRONOUS_PARAM(
    GAMMA, 65536, // 1 << 16
    "Control how fast you get from low to fast around the midpoint");
SYNCHRONOUS_PARAM(SMOOTH, 32768, // 0.5 << 16
                  "Control the suddeness of the sensitivity increase.");
SYNCHRONOUS_PARAM(
    MOTIVITY, 98304, // 1.5 << 16
    "Set the maximum sensitivity while also setting the minimum to "
    "1/MOTIVITY");
SYNCHRONOUS_PARAM(
    SYNC_SPEED, 327680, // 5 << 16
    "Set The middle sensitivity between you min and max sensitivity");
#endif

SYNCHRONOUS_PARAM(
    GAIN, 0, "Interpret the Synchronous curve as gain instead of sensitivity");

static DEFINE_MUTEX(synchronous_gain_update_lock);

static inline void rebuild_synchronous_gain_lut(void) {
  static struct synchronous_gain_lut next_lut;
  struct synchronous_curve_args args;

  mutex_lock(&synchronous_gain_update_lock);
  args = (struct synchronous_curve_args){
      .gamma = atofp(PARAM_GAMMA),
      .smooth = atofp(PARAM_SMOOTH),
      .motivity = atofp(PARAM_MOTIVITY),
      .sync_speed = atofp(PARAM_SYNC_SPEED),
      .gain = atofp(PARAM_GAIN),
  };
  __synchronous_gain_lut_init(&next_lut, args);
  publish_synchronous_gain_lut(&next_lut);
  mutex_unlock(&synchronous_gain_update_lock);
}

static int set_synchronous_param(const char *value,
                                 const struct kernel_param *param) {
  int error = param_set_charp(value, param);

  if (!error)
    rebuild_synchronous_gain_lut();
  return error;
}

static const struct kernel_param_ops synchronous_param_ops = {
    .set = set_synchronous_param,
    .get = param_get_charp,
};

// Flags
#define PARAM_FLAG(param, default_value, desc)                                 \
  unsigned char PARAM_##param = default_value;                                 \
  module_param_named(param, PARAM_##param, byte, RW_USER_GROUP);               \
  MODULE_PARM_DESC(param, desc);

PARAM_FLAG(MODE, linear, "Desired type of acceleration.");

#endif // !_PARAM_H_
