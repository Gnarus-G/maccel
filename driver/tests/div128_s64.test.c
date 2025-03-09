#include "../dbg.h"
#include "../fixedptc.h"
#include "test_utils.h"
#include <assert.h>
#include <stdint.h>

void test_custom_division_against_fixedpt(double a, double b) {
#if FIXEDPT_BITS == 32
  return;
#else

  fpt n = fpt_rconst(a);
  fpt divisor = fpt_rconst(b);

  fpt quotient = div128_s64_s64(n, divisor);
  fpt quotient1 = fpt_xdiv(n, divisor);

  double actual = fpt_todouble(quotient);
  double expected = fpt_todouble(quotient1);

  dbg("actual = (%li) -> %.10f", quotient, actual);
  dbg("expect = (%li) -> %.10f", quotient1, expected);

  assert(actual == expected);
#endif
}

int main(void) {

  test_custom_division_against_fixedpt(57, 5.5);

  test_custom_division_against_fixedpt(0, 2.57);

  test_custom_division_against_fixedpt(-1, 3);

  test_custom_division_against_fixedpt(-128, 4);

  test_custom_division_against_fixedpt(127, 1.5);

  /* test_custom_division_against_fixedpth(135, 0); */ // You only crash once!

  print_success;

  return 0;
}
