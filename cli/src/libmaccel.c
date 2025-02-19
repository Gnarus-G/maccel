#include "../../driver/accel_rs.h"
#include "../../driver/fixedptc.h"

extern char *fixedpt_to_str(fixedpt num) { return fptoa(num); }

extern double fixedpt_to_float(fixedpt value) {
  return fixedpt_todouble(value);
}

extern fixedpt fixedpt_from_float(double value) {
  return fixedpt_rconst(value);
}
