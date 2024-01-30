#include "fixedptc.h"
#include "linux/moduleparam.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default, desc)                                            \
  static fixedpt PARAM_##param = fixedpt_rconst(default);                      \
  module_param_named(param, PARAM_##param, int, RW_USER_GROUP);                \
  MODULE_PARM_DESC(param, desc);

PARAM(
    SENSITIVITY, 1.0,
    "Control the sensitivity, a factor applied after the acceleration after.");
PARAM(ACCEL, 0.0, "Control the acceleration factor.");
PARAM(OFFSET, 0.0, "Control the input speed past which to allow acceleration.");
PARAM(OUTPUT_CAP, 0.0, "Control the maximum acceleration factor.");
