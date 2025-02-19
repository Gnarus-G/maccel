#include "../accel.h"
#include "./test_utils.h"
#include <stdio.h>

int assert_string_value(char *filename, double x, double y, double dt) {
  fixedpt dx = fixedpt_rconst(x);
  fixedpt dy = fixedpt_rconst(y);

  dbg("in                        (%f, %f)", x, y);
  dbg("in: x (fixedpt conversion) %s", fptoa(x));
  dbg("in: y (fixedpt conversion) %s", fptoa(y));

  fixedpt s = input_speed(dx, dy, dt);

  double res = fixedpt_todouble(s);
  dbg("(%f, %f) dt = %f -> %f\n", x, y, dt, res);

  char content[100];
  sprintf(content, "(sqrt(%f, %f) / %f) = %f\n", x, y, dt, res);

  assert_snapshot(filename, content);

  return 0;
}

#define test(x, y, time)                                                       \
  assert(assert_string_value(__FILE_NAME__ "_sqrt_" #x "_" #y "_" #time        \
                                           ".snapshot",                        \
                             x, y, time) == 0)

int main(void) {
  test(1, 1, 1);
  test(1, 21, 1);
  test(64, -37, 1);
  test(1, 4, 1);

  test(-1, 1, 4);

  test(1, 0, 100);

  test(1, -1, 100);

  test(-1, -24, 1);

  print_success;
  return 0;
}
