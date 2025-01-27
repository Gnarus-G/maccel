#include "accel.h"

extern inline fixedpt sensitivity_rs(fixedpt input_speed, fixedpt param_sens_mult,
                                     fixedpt param_accel, fixedpt param_offset,
                                     fixedpt param_output_cap) {
  return sensitivity(input_speed, param_sens_mult, param_accel, param_offset, param_output_cap);
}
