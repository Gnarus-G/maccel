#include "test_utils.h"
#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int assert_string_value(char *filename, double value) {
  fixedpt v = fixedpt_rconst(value);
  char *_v = fptoa(v);

  dbg("to_string %f = %s", value, _v);

  assert_snapshot(filename, _v);
  return 0;
}

#define test_str(value)                                                        \
  assert(assert_string_value(__FILE_NAME__ "_" #value ".snapshot", value) == 0)

int main(void) {
  test_str(0.25);
  test_str(0.125);
  test_str(0.3125);
  test_str(-785);

  print_success;
}
