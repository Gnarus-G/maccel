#include "../fixedptc.h"
#include "test_utils.h"
#include <time.h>

#define atofp_test(name, value, expected)                                      \
  TEST name(void) {                                                            \
    fpt n = atofp(value);                                                      \
    double actual = fpt_todouble(n);                                           \
    dbg("actual: (%li) %.15f, vs expected: %.15f\n", n, actual, expected);     \
    ASSERT_EQ(expected, actual);                                               \
    PASS();                                                                    \
  }

atofp_test(converts_quarter, "1073741824", 0.25);
atofp_test(converts_eighth, "536870912", 0.125);
atofp_test(converts_five_sixteenths, "1342177280", 0.3125);
atofp_test(converts_negative_integer, "-335007449088", -78);

TEST reports_conversion_benchmark(void) {
  int iterations = 100000;
  double sum = 0;

  for (int i = 0; i < iterations; i++) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    atofp("1826512586328");
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    sum += (double)(end.tv_nsec - begin.tv_nsec);
  }

  printf("\n  Average conversion: %fns\n", sum / iterations);
  PASS();
}

SUITE(atofp_conversion) {
  RUN_TEST(converts_quarter);
  RUN_TEST(converts_eighth);
  RUN_TEST(converts_five_sixteenths);
  RUN_TEST(converts_negative_integer);
  RUN_TEST(reports_conversion_benchmark);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(atofp_conversion);
  GREATEST_MAIN_END();
}
