#ifndef _PARAM_H_
#define _PARAM_H_

#include "accel/mode.h"
#include "fixedptc.h"
#include "linux/moduleparam.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default_value, desc)                                      \
  char *PARAM_##param = #default_value;                                        \
  module_param_named(param, PARAM_##param, charp, RW_USER_GROUP);              \
  MODULE_PARM_DESC(param, desc);

#if FIXEDPT_BITS == 64
PARAM(
    SENS_MULT, 4294967296, // 1 << 32
    "A factor applied by the sensitivity calculation after ACCEL is applied.");
PARAM(YX_RATIO, 4294967296, // 1 << 32
      "A factor (Y/X) by which the final sensitivity calculated is multiplied "
      "to produce the sensitivity applied to the Y axis.");
PARAM(INPUT_DPI, 4294967296000, // 1000 << 32
      "The DPI of the mouse, used to normalize the effective dpi to 1 "
      "in/sec");
#else
PARAM(SENS_MULT, 65536, // 1 << 16
      "A factor applied the sensitivity calculation after ACCEL is applied.");
PARAM(YX_RATIO, 65536, // 1 << 16
      "A factor (Y/X) by which the final sensitivity calculated is multiplied "
      "to produce the sensitivity applied to the Y axis.");
PARAM(INPUT_DPI, 65536000, // 1000 << 16
      "The DPI of the mouse, used to normalize the effective dpi to 1 "
      "in/sec");
#endif

PARAM(ACCEL, 0, "Control the sensitivity calculation.");
PARAM(OFFSET, 0, "Control the input speed past which to allow acceleration.");
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

PARAM(GAMMA, 4294967296, // 1 << 32
      "Control how fast you get from low to fast around the midpoint");
PARAM(SMOOTH, 2147483648, // 0.5 << 32
      "Control the suddeness of the sensitivity increase.");
PARAM(MOTIVITY, 6442450944, // 1.5 << 32
      "Set the maximum sensitivity while also setting the minimum to "
      "1/MOTIVITY");
PARAM(SYNC_SPEED, 21474836480, // 5 << 32
      "Set The middle sensitivity between you min and max sensitivity");

// Flags
#define PARAM_FLAG(param, default_value, desc)                                 \
  unsigned char PARAM_##param = default_value;                                 \
  module_param_named(param, PARAM_##param, byte, RW_USER_GROUP);               \
  MODULE_PARM_DESC(param, desc);

PARAM_FLAG(MODE, linear, "Desired type of acceleration.");

#endif // !_PARAM_H_
