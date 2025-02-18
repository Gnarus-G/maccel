for test in tests/*.test.c; do
  gcc ${test} -o maccel_test -g
  ./maccel_test
  rm maccel_test
done
