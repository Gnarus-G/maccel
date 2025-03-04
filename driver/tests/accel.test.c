#include "../accel.h"
#include "test_utils.h"
#include <stdio.h>

static int test_acceleration(const char *filename, struct accel_args args) {
  const int LINE_LEN = 26;
  const int MIN = -128;
  const int MAX = 127;

  char content[256 * 256 * LINE_LEN + 1];
  strcpy(content, ""); // initialize as an empty string

  for (int x = MIN; x < MAX; x++) {
    for (int y = MIN; y < MAX; y++) {

      int x_out = x;
      int y_out = y;

      f_accelerate(&x_out, &y_out, FIXEDPT_ONE, args);

      char curr_debug_print[LINE_LEN];

      sprintf(curr_debug_print, "(%d, %d) => (%d, %d)\n", x, y, x_out, y_out);

      strcat(content, curr_debug_print);
    }
  }

  assert_snapshot(filename, content);

  return 0;
}

static int test_linear_acceleration(const char *filename,
                                    fixedpt param_sens_mult,
                                    fixedpt param_yx_ratio, fixedpt param_accel,
                                    fixedpt param_offset,
                                    fixedpt param_output_cap) {
  struct linear_curve_args _args =
      (struct linear_curve_args){.accel = param_accel,
                                 .offset = param_offset,
                                 .output_cap = param_output_cap};

  struct accel_args args = {
      .param_sens_mult = param_sens_mult,
      .param_yx_ratio = param_yx_ratio,
      .tag = linear,
      .args = (union __accel_args){.linear = _args},
  };

  return test_acceleration(filename, args);
}

static int
test_natural_acceleration(const char *filename, fixedpt param_sens_mult,
                          fixedpt param_yx_ratio, fixedpt param_decay_rate,
                          fixedpt param_offset, fixedpt param_limit) {
  struct natural_curve_args _args =
      (struct natural_curve_args){.decay_rate = param_decay_rate,
                                  .offset = param_offset,
                                  .limit = param_limit};

  struct accel_args args = {
      .param_sens_mult = param_sens_mult,
      .param_yx_ratio = param_yx_ratio,
      .tag = natural,
      .args = (union __accel_args){.natural = _args},
  };

  return test_acceleration(filename, args);
}

#define test_linear(sens_mult, yx_ratio, accel, offset, cap)                   \
  assert(test_linear_acceleration(                                             \
             "SENS_MULT-" #sens_mult "-ACCEL-" #accel "-OFFSET" #offset        \
             "-OUTPUT_CAP-" #cap ".snapshot",                                  \
             fixedpt_rconst(sens_mult), fixedpt_rconst(yx_ratio),              \
             fixedpt_rconst(accel), fixedpt_rconst(offset),                    \
             fixedpt_rconst(cap)) == 0);

#define test_natural(sens_mult, yx_ratio, decay_rate, offset, limit)           \
  assert(test_linear_acceleration(                                             \
             "Natural__SENS_MULT-" #sens_mult "-ACCEL-" #decay_rate            \
             "-OFFSET" #offset "-LIMIT-" #limit ".snapshot",                   \
             fixedpt_rconst(sens_mult), fixedpt_rconst(yx_ratio),              \
             fixedpt_rconst(decay_rate), fixedpt_rconst(offset),               \
             fixedpt_rconst(limit)) == 0);

int main(void) {
  test_linear(1, 1, 0, 0, 0);
  test_linear(1, 1, 0.3, 2, 2);
  test_linear(0.1325, 1, 0.3, 21.333333, 2);
  test_linear(0.1875, 1, 0.05625, 10.6666666, 2);
  test_linear(0.0917, 1, 0.002048, 78.125, 2.0239);
  test_linear(0.07, 1.15, 0.055, 21, 3);

  test_natural(1, 1, 0, 0, 0);
  test_natural(1, 1, 0.1, 0, 0);
  test_natural(1, 1, 0.1, 8, 0);
  test_natural(1, 1, 0.03, 8, 1.5);

  print_success;
}
