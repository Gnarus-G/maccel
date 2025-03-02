#include "accel.h"

extern inline struct vector
sensitivity_rs(fixedpt input_speed, fixedpt param_sens_mult,
               fixedpt param_yx_ratio, fixedpt param_accel,
               fixedpt param_offset, fixedpt param_output_cap) {
  return sensitivity(input_speed, param_sens_mult, param_yx_ratio, param_accel,
                     param_offset, param_output_cap);
}
