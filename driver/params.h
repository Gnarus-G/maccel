#ifndef _PARAM_H_
#define _PARAM_H_

#include "fixedptc.h"
#include "linux/moduleparam.h"

#define RW_USER_GROUP 0664

#define PARAM(param, default, desc)                                            \
  static fixedpt PARAM_##param = fixedpt_rconst(default);                      \
  module_param_named(param, PARAM_##param, int, RW_USER_GROUP);                \
  MODULE_PARM_DESC(param, desc);

PARAM(SENS_MULT, 1,
      "A factor applied the sensitivity calculation after ACCEL is applied.");
PARAM(ACCEL, 0, "Control the sensitivity calculation.");

PARAM(MOTIVITY, 4, "Motivity setting, defines scale of difference between min and max sens.");
PARAM(SYNC_SPEED, 31, "Midpoint/syncspeed for motivity.");
PARAM(GAMMA, 0.8, "Gamma/growth for motivity.");

PARAM(OFFSET, 0, "Control the input speed past which to allow acceleration.");
PARAM(OUTPUT_CAP, 0, "Control the maximum sensitivity.");

#endif // !_PARAM_H_
