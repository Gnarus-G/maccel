#include "../fixedptc.h"
#include "test_utils.h"

static int assert_string_value(char *filename, double value) {
  fpt v = fpt_rconst(value);
  char *actual = fptoa(v);

  dbg("to_string %f = %s", value, actual);
  return assert_snapshot(filename, actual);
}

#define fp_to_str_test(name, value)                                            \
  TEST name(void) {                                                            \
    ASSERT_EQm(                                                                \
        "snapshot differs: " __FILE_NAME__ "_" #value ".snapshot", 0,          \
        assert_string_value(__FILE_NAME__ "_" #value ".snapshot", value));     \
    PASS();                                                                    \
  }

fp_to_str_test(converts_quarter, 0.25);
fp_to_str_test(converts_eighth, 0.125);
fp_to_str_test(converts_five_sixteenths, 0.3125);
fp_to_str_test(converts_negative_integer, -785);

SUITE(fp_to_str) {
  RUN_TEST(converts_quarter);
  RUN_TEST(converts_eighth);
  RUN_TEST(converts_five_sixteenths);
  RUN_TEST(converts_negative_integer);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(fp_to_str);
  GREATEST_MAIN_END();
}
