#include "test_utils.h"
#include <stdio.h>

int main(void) {
  test(1, 0.3, 2, 2);
  test(0.1325, 0.3, 21.333333, 2);
  test(0.1875, 0.05625, 10.6666666, 2);

  test(0.0917, 0.002048, 78.125, 2.0239);

  printf("[accel] All tests passed!\n");
}
