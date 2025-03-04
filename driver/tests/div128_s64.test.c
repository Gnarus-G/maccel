#include "../dbg.h"
#include "../fixedptc.h"
#include "test_utils.h"
#include <assert.h>
#include <stdint.h>

void test_custom_division_against_fixedpth(double a, double b) {
#if FIXEDPT_BITS == 32
  return;
#else

  fixedpt n = fixedpt_rconst(a);
  fixedpt divisor = fixedpt_rconst(b);

  fixedpt quotient = div128_s64_s64(n, divisor);
  fixedpt quotient1 = fixedpt_xdiv(n, divisor);

  double actual = fixedpt_todouble(quotient);
  double expected = fixedpt_todouble(quotient1);

  dbg("actual = (%li) -> %.10f", quotient, actual);
  dbg("expect = (%li) -> %.10f", quotient1, expected);

  assert(actual == expected);
#endif
}

int main(void) {

  test_custom_division_against_fixedpth(57, 5.5);

  test_custom_division_against_fixedpth(0, 2.57);

  test_custom_division_against_fixedpth(-1, 3);

  test_custom_division_against_fixedpth(-128, 4);

  test_custom_division_against_fixedpth(127, 1.5);

  /* test_custom_division_against_fixedpth(135, 0); */ // You only crash once!

  print_success;

  return 0;
}
