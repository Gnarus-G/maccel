#include "fixedptc.h"
#include "linux/moduleparam.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default, desc)                                            \
  static fixedpt PARAM_##param = fixedpt_rconst(default);                      \
  module_param_named(param, PARAM_##param, int, RW_USER_GROUP);                \
  MODULE_PARM_DESC(param, desc);

PARAM(ACCEL, 0.3, "Control the acceleration factor.");
PARAM(OFFSET, 2, "Control the input speed past which to allow acceleration.");
PARAM(OUTPUT_CAP, 2, "Control the maximum output speed.");
