#include "../accel.h"
#include "test_utils.h"
#include <sys/wait.h>

/*
 * Encode the proper behavior on invalid timing / divisors: the call must
 * complete (no SIGFPE in userspace, no #DE in kernel). Each case runs in a
 * forked child because the current implementation traps with SIGFPE; the
 * guard makes these pass.
 */

static int run_child_exit(void (*fn)(void)) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    return -1;
  }
  if (pid == 0) {
    /* Silence stdout so dbg() noise from a crashing child doesn't pollute
     * the Greatest report; only the child's exit status is observed. */
    freopen("/dev/null", "w", stdout);
    fn();
    _exit(0);
  }
  int status;
  pid_t result;
  do {
    result = waitpid(pid, &status, 0);
  } while (result == -1 && errno == EINTR);
  if (result == -1) {
    perror("waitpid");
    return -1;
  }
  return status;
}

static struct accel_args default_linear_args(void) {
  struct linear_curve_args lin = {
      .accel = fpt_rconst(0.3), .offset = fpt_rconst(2), .output_cap = 0};
  struct accel_args args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .input_dpi = fpt_fromint(1000),
      .angle_rotation_deg = 0,
      .tag = linear,
      .args = (union __accel_args){.linear = lin},
  };
  return args;
}

static struct accel_args default_synchronous_args(void) {
  struct synchronous_curve_args sync = {.gamma = fpt_rconst(0.8),
                                        .smooth = fpt_rconst(0.5),
                                        .motivity = fpt_rconst(1.5),
                                        .sync_speed = fpt_rconst(32)};
  struct accel_args args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .input_dpi = fpt_fromint(1000),
      .angle_rotation_deg = 0,
      .tag = synchronous,
      .args = (union __accel_args){.synchronous = sync},
  };
  return args;
}

/* --- crash cases: run each in a child and require clean survival --- */

static void child_input_speed_zero_time(void) {
  fpt speed = input_speed(fpt_fromint(10), fpt_fromint(5), 0);
  if (speed != 0)
    _exit(2);
}

TEST input_speed_time_zero_returns_zero(void) {
  int status = run_child_exit(child_input_speed_zero_time);
  ASSERT_EQ_FMTm("input_speed(time=0) should return zero", 0, status, "%d");
  PASS();
}

static void child_input_dpi_zero(void) {
  struct accel_args args = default_linear_args();
  args.input_dpi = 0;
  int x = 10, y = 5;
  f_accelerate(&x, &y, FIXEDPT_ONE, args);
}

TEST faccelerate_input_dpi_zero_survives(void) {
  int status = run_child_exit(child_input_dpi_zero);
  ASSERT_EQ_FMTm("f_accelerate(input_dpi=0) should not crash", 0, status, "%d");
  PASS();
}

static void child_motivity_one(void) {
  struct accel_args args = default_synchronous_args();
  args.args.synchronous.motivity = FIXEDPT_ONE; /* ln(1) == 0 divisor */
  int x = 10, y = 5;
  f_accelerate(&x, &y, FIXEDPT_ONE, args);
}

TEST synchronous_motivity_one_survives(void) {
  int status = run_child_exit(child_motivity_one);
  ASSERT_EQ_FMTm("synchronous(motivity=1) should not crash", 0, status, "%d");
  PASS();
}

/* --- asserted values --- */

static void child_faccelerate_time_zero_identity(void) {
  struct accel_args args = default_linear_args();
  int x = 10, y = 5;
  f_accelerate(&x, &y, 0, args);
  /* encode the intended behavior: no acceleration on a zero-time frame */
  if (x != 10 || y != 5)
    _exit(2);
}

TEST faccelerate_time_zero_identity(void) {
  int status = run_child_exit(child_faccelerate_time_zero_identity);
  ASSERT_EQ_FMTm("should not crash", 0, status, "%d");
  PASS();
}

static void child_input_speed_negative_time(void) {
  fpt speed = input_speed(fpt_fromint(10), fpt_fromint(5), -FIXEDPT_ONE);
  if (speed != 0)
    _exit(3);
}

TEST input_speed_negative_time_returns_zero(void) {
  int status = run_child_exit(child_input_speed_negative_time);
  ASSERT_EQ_FMTm("input_speed(time<0) should return zero", 0, status, "%d");
  PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_TEST(input_speed_time_zero_returns_zero);
  RUN_TEST(faccelerate_input_dpi_zero_survives);
  RUN_TEST(synchronous_motivity_one_survives);
  RUN_TEST(faccelerate_time_zero_identity);
  RUN_TEST(input_speed_negative_time_returns_zero);
  GREATEST_MAIN_END();
}
