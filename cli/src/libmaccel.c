#include "../../driver/accel_rs.h"
#include "../../driver/fixedptc.h"

char *fixedpt_to_str(fixedpt num);
fixedpt str_to_fixedpt(char *string);
double fixedpt_to_float(fixedpt value);
fixedpt fixedpt_from_float(double value);

extern char *fixedpt_to_str(fixedpt num) { return fptoa(num); }
extern fixedpt str_to_fixedpt(char *string) { return atofp(string); }

extern double fixedpt_to_float(fixedpt value) {
  return fixedpt_todouble(value);
}

extern fixedpt fixedpt_from_float(double value) {
  return fixedpt_rconst(value);
}
