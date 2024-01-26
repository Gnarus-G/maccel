#include "../../driver/accel.h"
#include "../../driver/fixedptc.h"

extern char *fixedpt_to_str(fixedpt num) { return fixedpt_cstr(num, 3); }

extern fixedpt fixedpt_from_float(float value) { return fixedpt_rconst(value); }

extern float fixedpt_to_float(fixedpt value) { return fixedpt_tofloat(value); }
