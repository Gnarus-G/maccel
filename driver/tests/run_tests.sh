#!/bin/bash

for test in tests/*.test.c; do
  if [[ ! $test =~ $TEST_NAME ]]; then
    continue
  fi
  gcc ${test} -o maccel_test -lm $EXTRA_CFLAGS
  ./maccel_test
  rm maccel_test
done
