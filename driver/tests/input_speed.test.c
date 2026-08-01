#include "../speed.h"
#include "test_utils.h"

static int assert_string_value(char *filename, double x, double y, double t) {
  fpt dx = fpt_rconst(x);
  fpt dy = fpt_rconst(y);
  fpt dt = fpt_rconst(t);
  fpt speed = input_speed(dx, dy, dt);
  double result = fpt_todouble(speed);
  char content[100];

  dbg("in (%f, %f)", x, y);
  dbg("in: x (fpt conversion) %s", fptoa(x));
  dbg("in: y (fpt conversion) %s", fptoa(y));
  dbg("(%f, %f) dt = %f -> %f\n", x, y, t, result);
  sprintf(content, "(sqrt(%f, %f) / %f) = %f\n", x, y, t, result);

  return assert_snapshot(filename, content);
}

#define input_speed_test(name, x, y, time)                                     \
  TEST name(void) {                                                            \
    ASSERT_EQm("snapshot differs: " __FILE_NAME__ "_sqrt_" #x "_" #y "_" #time \
               ".snapshot",                                                    \
               0,                                                              \
               assert_string_value(__FILE_NAME__ "_sqrt_" #x "_" #y "_" #time  \
                                                 ".snapshot",                  \
                                   x, y, time));                               \
    PASS();                                                                    \
  }

input_speed_test(speed_1_1_1, 1, 1, 1);
input_speed_test(speed_1_21_1, 1, 21, 1);
input_speed_test(speed_64_negative_37_1, 64, -37, 1);
input_speed_test(speed_1_4_1, 1, 4, 1);
input_speed_test(speed_negative_1_1_4, -1, 1, 4);
input_speed_test(speed_1_0_100, 1, 0, 100);
input_speed_test(speed_1_negative_1_100, 1, -1, 100);
input_speed_test(speed_negative_1_negative_24_1, -1, -24, 1);

SUITE(input_speed_snapshots) {
  RUN_TEST(speed_1_1_1);
  RUN_TEST(speed_1_21_1);
  RUN_TEST(speed_64_negative_37_1);
  RUN_TEST(speed_1_4_1);
  RUN_TEST(speed_negative_1_1_4);
  RUN_TEST(speed_1_0_100);
  RUN_TEST(speed_1_negative_1_100);
  RUN_TEST(speed_negative_1_negative_24_1);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(input_speed_snapshots);
  GREATEST_MAIN_END();
}
