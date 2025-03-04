#include "accel.h"

extern inline struct vector sensitivity_rs(fixedpt input_speed,
                                           struct accel_args args) {
  return sensitivity(input_speed, args);
}
