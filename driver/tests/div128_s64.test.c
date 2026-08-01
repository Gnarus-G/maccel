#include "../dbg.h"
#include "../fixedptc.h"
#include "test_utils.h"

#define division_test(name, a, b)                                              \
  TEST name(void) {                                                            \
    fpt n = fpt_rconst(a);                                                     \
    fpt divisor = fpt_rconst(b);                                               \
    fpt quotient = div128_s64_s64(n, divisor);                                 \
    fpt expected_quotient = fpt_xdiv(n, divisor);                              \
    double actual = fpt_todouble(quotient);                                    \
    double expected = fpt_todouble(expected_quotient);                         \
    dbg("actual = (%li) -> %.10f", quotient, actual);                          \
    dbg("expect = (%li) -> %.10f", expected_quotient, expected);               \
    ASSERT_EQ(expected, actual);                                               \
    PASS();                                                                    \
  }

#if FIXEDPT_BITS == 32
TEST division_is_not_supported(void) { SKIP(); }
#else
division_test(divides_positive_values, 57, 5.5);
division_test(divides_zero, 0, 2.57);
division_test(divides_negative_value, -1, 3);
division_test(divides_negative_exactly, -128, 4);
division_test(divides_positive_fraction, 127, 1.5);
#endif

SUITE(signed_128_bit_division) {
#if FIXEDPT_BITS == 32
  RUN_TEST(division_is_not_supported);
#else
  RUN_TEST(divides_positive_values);
  RUN_TEST(divides_zero);
  RUN_TEST(divides_negative_value);
  RUN_TEST(divides_negative_exactly);
  RUN_TEST(divides_positive_fraction);
#endif
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(signed_128_bit_division);
  GREATEST_MAIN_END();
}
