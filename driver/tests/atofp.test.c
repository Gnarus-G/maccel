#include "../utils.h"
#include <assert.h>
#include <stdlib.h>

void test_eq(char *value, float expected) {
  fixedpt n = atofp(value);
  float actual = fixedpt_tofloat(n);
  dbg("actual: %f, vs expected: %f\n", actual, expected);
  assert(actual == expected);
}

int main(void) {
  test_eq("8060928", 123.0);
  test_eq("20480", 0.3125);
  test_eq("8819920", 134.581299);

  printf("[atofp] All tests passed!\n");
}
