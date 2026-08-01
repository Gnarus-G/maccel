#include "../dbg.h"
#include "../fixedptc.h"
#include "test_utils.h"
#include <sys/wait.h>

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

/*
 * A zero divisor currently raises SIGFPE in the raw idivq helper. The guard
 * must make division by zero *return* a defined value instead of trapping, so
 * run it in a forked child and require a clean exit (red now, green after the
 * guard). This keeps the failure an assertion instead of killing the suite.
 */
TEST divides_zero_divisor(void) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    FAILm("fork failed");
  }
  if (pid == 0) {
    freopen("/dev/null", "w", stdout);
    div128_s64_s64(fpt_rconst(127), 0);
    _exit(0);
  }
  int status;
  waitpid(pid, &status, 0);
  ASSERT_EQ_FMTm("division by zero should not crash", 0, status, "%d");
  PASS();
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
  RUN_TEST(divides_zero_divisor);
#endif
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(signed_128_bit_division);
  GREATEST_MAIN_END();
}
