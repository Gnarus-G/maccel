#!/bin/bash

for test in tests/*.test.c; do
  if [[ ! $test =~ $TEST_NAME ]]; then
    continue
  fi
  gcc ${test} -o maccel_test -lm $DRIVER_CFLAGS || exit 1
  ./maccel_test || exit 1
  rm maccel_test
done
