#include "../../../driver/accel_rs.h"
#include "../../../driver/fixedptc.h"

char *fpt_to_str(fpt num);
fpt str_to_fpt(char *string);
double fpt_to_float(fpt value);
fpt fpt_from_float(double value);

extern char *fpt_to_str(fpt num) { return fptoa(num); }
extern fpt str_to_fpt(char *string) { return atofp(string); }

extern double fpt_to_float(fpt value) { return fpt_todouble(value); }

extern fpt fpt_from_float(double value) { return fpt_rconst(value); }
