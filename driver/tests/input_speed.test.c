#include "../speed.h"
#include "./test_utils.h"
#include <stdio.h>

int assert_string_value(char *filename, double x, double y, double t) {
  fpt dx = fpt_rconst(x);
  fpt dy = fpt_rconst(y);
  fpt dt = fpt_rconst(t);

  dbg("in                        (%f, %f)", x, y);
  dbg("in: x (fpt conversion) %s", fptoa(x));
  dbg("in: y (fpt conversion) %s", fptoa(y));

  fpt s = input_speed(dx, dy, dt);

  double res = fpt_todouble(s);
  dbg("(%f, %f) dt = %f -> %f\n", x, y, t, res);

  char content[100];
  sprintf(content, "(sqrt(%f, %f) / %f) = %f\n", x, y, t, res);

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
