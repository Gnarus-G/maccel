#include "../utils.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

void test_eq(char *value, float expected) {
  fixedpt n = atofp(value);
  float actual = fixedpt_tofloat(n);
  dbg("actual: %f, vs expected: %f\n", actual, expected);
  assert(actual == expected);
}

void super_tiny_micro_minuscule_bench() {
  int iterations = 100000;
  double sum = 0;
  for (int i = 0; i < iterations; i++) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    fixedpt n = atofp("1826512586328");
    dbg("atofp(\"1826512586328\") = %d\n", n);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    sum += (double)(end.tv_nsec - begin.tv_nsec);
  }

  double avg = sum / iterations;
  printf("[afofp] Avg run time is %fns\n", avg);
}

int main(void) {
  test_eq("8060928", 123.0);
  test_eq("20480", 0.3125);
  test_eq("8819920", 134.581299);

  printf("[atofp] All tests passed!\n");

  super_tiny_micro_minuscule_bench();
}
