#include "fixedptc.h"
#include "linux/moduleparam.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default, desc)                                            \
  static fixedpt PARAM_##param = fixedpt_rconst(default);                      \
  module_param_named(param, PARAM_##param, int, RW_USER_GROUP);                \
  MODULE_PARM_DESC(param, desc);

PARAM(SENS_MULT, 0.09375,
      "A factor applied the sensitivity calculation after ACCEL is applied.");
PARAM(ACCEL, 0.05, "Control the sensitivity calculation.");
PARAM(OFFSET, 21.333333,
      "Control the input speed past which to allow acceleration.");
PARAM(OUTPUT_CAP, 2.0, "Control the maximum sensitivity.");
