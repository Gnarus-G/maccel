#include "test_utils.h"
#include <stdio.h>

static int test_acceleration(const char *filename, fixedpt param_sens_mult,
                             fixedpt param_accel, fixedpt param_offset,
                             fixedpt param_output_cap) {
  const int LINE_LEN = 26;
  const int MIN = -128;
  const int MAX = 127;

  char content[256 * 256 * LINE_LEN + 1];
  strcpy(content, ""); // initialize as an empty string

  AccelResult result;
  for (int x = MIN; x < MAX; x++) {
    for (int y = MIN; y < MAX; y++) {

      result = f_accelerate(x, y, 1, param_sens_mult, param_accel, param_offset,
                            param_output_cap);
      char curr_debug_print[LINE_LEN];

      sprintf(curr_debug_print, "(%d, %d) => (%d, %d)\n", x, y, result.x,
              result.y);

      strcat(content, curr_debug_print);
    }
  }

  assert_snapshot(filename, content);

  return 0;
}

#define test(sens_mult, accel, offset, cap)                                    \
  assert(test_acceleration("SENS_MULT-" #sens_mult "-ACCEL-" #accel            \
                           "-OFFSET" #offset "-OUTPUT_CAP-" #cap ".snapshot",  \
                           fixedpt_rconst(sens_mult), fixedpt_rconst(accel),   \
                           fixedpt_rconst(offset), fixedpt_rconst(cap)) == 0);

int main(void) {
  test(1, 0.3, 2, 2);
  test(0.1325, 0.3, 21.333333, 2);
  test(0.1875, 0.05625, 10.6666666, 2);

  test(0.0917, 0.002048, 78.125, 2.0239);

  print_success;
}
