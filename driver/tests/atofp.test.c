#include "../fixedptc.h"
#include "./test_utils.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

void test_eq(char *value, double expected) {
  fixedpt n = atofp(value);
  double actual = fixedpt_todouble(n);
  dbg("actual: (%li) %.15f, vs expected: %.15f\n", n, actual, expected);
  assert(actual == expected);
}

void super_tiny_micro_minuscule_bench() {
  int iterations = 100000;
  double sum = 0;
  for (int i = 0; i < iterations; i++) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    fixedpt n = atofp("1826512586328");
    dbg("atofp(\"1826512586328\") = %li\n", n);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    sum += (double)(end.tv_nsec - begin.tv_nsec);
  }

  double avg = sum / iterations;
  printf("   Avg run time is %fns\n", avg);
}

int main(void) {
  test_eq("1073741824", 0.25);
  test_eq("536870912", 0.125);
  test_eq("1342177280", 0.3125);
  test_eq("-335007449088", -78);

  print_success;

  super_tiny_micro_minuscule_bench();
}
