#include "../accel.h"
#include "test_utils.h"
#include <stdio.h>

static int test_acceleration(const char *filename, struct accel_args args) {
  const int LINE_LEN = 26;
  const int MIN = -128;
  const int MAX = 127;

  char content[256 * 256 * LINE_LEN + 1];
  strcpy(content, ""); // initialize as an empty string

  for (int x = MIN; x < MAX; x++) {
    for (int y = MIN; y < MAX; y++) {

      int x_out = x;
      int y_out = y;

      f_accelerate(&x_out, &y_out, FIXEDPT_ONE, args);

      char curr_debug_print[LINE_LEN];

      sprintf(curr_debug_print, "(%d, %d) => (%d, %d)\n", x, y, x_out, y_out);

      strcat(content, curr_debug_print);
    }
  }

  return assert_snapshot(filename, content);
}

static int test_linear_acceleration(const char *filename, fpt param_sens_mult,
                                    fpt param_yx_ratio, fpt param_accel,
                                    fpt param_offset, fpt param_output_cap) {
  struct linear_curve_args _args =
      (struct linear_curve_args){.accel = param_accel,
                                 .offset = param_offset,
                                 .output_cap = param_output_cap};

  struct accel_args args = {
      .sens_mult = param_sens_mult,
      .yx_ratio = param_yx_ratio,
      .input_dpi = fpt_fromint(1000),
      .tag = linear,
      .args = (union __accel_args){.linear = _args},
  };

  return test_acceleration(filename, args);
}

static int test_natural_acceleration(const char *filename, fpt param_sens_mult,
                                     fpt param_yx_ratio, fpt param_decay_rate,
                                     fpt param_offset, fpt param_limit) {
  struct natural_curve_args _args =
      (struct natural_curve_args){.decay_rate = param_decay_rate,
                                  .offset = param_offset,
                                  .limit = param_limit};

  struct accel_args args = {
      .sens_mult = param_sens_mult,
      .yx_ratio = param_yx_ratio,
      .input_dpi = fpt_fromint(1000),
      .tag = natural,
      .args = (union __accel_args){.natural = _args},
  };

  return test_acceleration(filename, args);
}

static int test_synchronous_acceleration(const char *filename,
                                         fpt param_sens_mult,
                                         fpt param_yx_ratio, fpt param_gamma,
                                         fpt param_smooth, fpt param_motivity,
                                         fpt param_sync_speed) {
  struct synchronous_curve_args _args =
      (struct synchronous_curve_args){.gamma = param_gamma,
                                      .smooth = param_smooth,
                                      .motivity = param_motivity,
                                      .sync_speed = param_sync_speed};

  struct accel_args args = {
      .sens_mult = param_sens_mult,
      .yx_ratio = param_yx_ratio,
      .input_dpi = fpt_fromint(1000),
      .tag = synchronous,
      .args = (union __accel_args){.synchronous = _args},
  };

  return test_acceleration(filename, args);
}

static int test_no_accel_acceleration(const char *filename, fpt param_sens_mult,
                                      fpt param_yx_ratio) {
  struct no_accel_curve_args _args = (struct no_accel_curve_args){};

  struct accel_args args = {
      .sens_mult = param_sens_mult,
      .yx_ratio = param_yx_ratio,
      .input_dpi = fpt_fromint(1000),
      .tag = no_accel,
      .args = (union __accel_args){.no_accel = _args},
  };

  return test_acceleration(filename, args);
}

static int test_rotation_no_accel(const char *filename, fpt param_sens_mult,
                                  fpt param_angle_deg) {
  struct no_accel_curve_args _args = (struct no_accel_curve_args){};

  struct accel_args args = {
      .sens_mult = param_sens_mult,
      .yx_ratio = FIXEDPT_ONE,
      .input_dpi = fpt_fromint(1000),
      .angle_rotation_deg = param_angle_deg,
      .tag = no_accel,
      .args = (union __accel_args){.no_accel = _args},
  };

  return test_acceleration(filename, args);
}

#define linear_test(name, sens_mult, yx_ratio, accel, offset, cap)             \
  TEST name(void) {                                                            \
    ASSERT_EQ(0, test_linear_acceleration(                                     \
                     "SENS_MULT-" #sens_mult "-ACCEL-" #accel                  \
                     "-OFFSET" #offset "-OUTPUT_CAP-" #cap ".snapshot",        \
                     fpt_rconst(sens_mult), fpt_rconst(yx_ratio),              \
                     fpt_rconst(accel), fpt_rconst(offset), fpt_rconst(cap))); \
    PASS();                                                                    \
  }

#define natural_test(name, sens_mult, yx_ratio, decay_rate, offset, limit)     \
  TEST name(void) {                                                            \
    ASSERT_EQ(0,                                                               \
              test_natural_acceleration(                                       \
                  "Natural__SENS_MULT-" #sens_mult "-DECAY_RATE-" #decay_rate  \
                  "-OFFSET" #offset "-LIMIT-" #limit ".snapshot",              \
                  fpt_rconst(sens_mult), fpt_rconst(yx_ratio),                 \
                  fpt_rconst(decay_rate), fpt_rconst(offset),                  \
                  fpt_rconst(limit)));                                         \
    PASS();                                                                    \
  }

#define synchronous_test(name, sens_mult, yx_ratio, gamma, smooth, motivity,   \
                         sync_speed)                                           \
  TEST name(void) {                                                            \
    ASSERT_EQ(0, test_synchronous_acceleration(                                \
                     "Synchronous__SENS_MULT-" #sens_mult "-GAMMA-" #gamma     \
                     "-SMOOTH" #smooth "-MOTIVITY-" #motivity                  \
                     "-SYNC_SPEED-" #sync_speed ".snapshot",                   \
                     fpt_rconst(sens_mult), fpt_rconst(yx_ratio),              \
                     fpt_rconst(gamma), fpt_rconst(smooth),                    \
                     fpt_rconst(motivity), fpt_rconst(sync_speed)));           \
    PASS();                                                                    \
  }

#define no_accel_test(name, sens_mult, yx_ratio)                               \
  TEST name(void) {                                                            \
    ASSERT_EQ(0, test_no_accel_acceleration(                                   \
                     "NoAccel__SENS_MULT-" #sens_mult "-YX_RATIO-" #yx_ratio   \
                     ".snapshot",                                              \
                     fpt_rconst(sens_mult), fpt_rconst(yx_ratio)));            \
    PASS();                                                                    \
  }

#define rotation_test(name, sens_mult, angle_deg)                              \
  TEST name(void) {                                                            \
    ASSERT_EQ(0, test_rotation_no_accel("Rotation__SENS_MULT-" #sens_mult      \
                                        "-ANGLE-" #angle_deg ".snapshot",      \
                                        fpt_rconst(sens_mult),                 \
                                        fpt_rconst(angle_deg)));               \
    PASS();                                                                    \
  }

linear_test(linear_identity, 1, 1, 0, 0, 0);
linear_test(linear_default, 1, 1, 0.3, 2, 2);
linear_test(linear_low_sensitivity, 0.1325, 1, 0.3, 21.333333, 2);
linear_test(linear_moderate_acceleration, 0.1875, 1, 0.05625, 10.6666666, 2);
linear_test(linear_low_acceleration, 0.0917, 1, 0.002048, 78.125, 2.0239);
linear_test(linear_yx_ratio, 0.07, 1.15, 0.055, 21, 3);
natural_test(natural_identity, 1, 1, 0, 0, 0);
natural_test(natural_decay, 1, 1, 0.1, 0, 0);
natural_test(natural_offset, 1, 1, 0.1, 8, 0);
natural_test(natural_limit, 1, 1, 0.03, 8, 1.5);
synchronous_test(synchronous_default, 1, 1.15, 0.8, 0.5, 1.5, 32);
no_accel_test(no_accel_identity, 1, 1);
no_accel_test(no_accel_yx_ratio, 0.5, 1.5);
rotation_test(rotation_45_degrees, 1, 45);
rotation_test(rotation_90_degrees, 1, 90);

TEST angle_snap_horizontal(void) {
  struct accel_args args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .input_dpi = fpt_fromint(1000),
      .angle_snap_threshold = snap_threshold(fpt_fromint(10)),
      .tag = no_accel,
  };
  int x = 10;
  int y = 1;

  f_accelerate(&x, &y, FIXEDPT_ONE, args);

  ASSERT_EQ(10, x);
  ASSERT_EQ(0, y);
  PASS();
}

TEST angle_snap_vertical(void) {
  struct vector snapped = snap_to_axis(
      (struct vector){fpt_fromint(1), fpt_fromint(-10)},
      snap_threshold(fpt_fromint(10)));

  ASSERT_EQ(0, snapped.x);
  ASSERT(snapped.y < 0);
  fpt expected = magnitude((struct vector){fpt_fromint(1), fpt_fromint(-10)});
  ASSERT(fpt_abs(expected - fpt_abs(snapped.y)) < fpt_rconst(0.0001));
  PASS();
}

TEST angle_snap_leaves_movement_outside_threshold(void) {
  struct vector input = {fpt_fromint(10), fpt_fromint(2)};
  struct vector snapped = snap_to_axis(input, snap_threshold(fpt_fromint(10)));

  ASSERT_EQ(input.x, snapped.x);
  ASSERT_EQ(input.y, snapped.y);
  PASS();
}

TEST angle_snap_leaves_diagonal_at_forty_five_degrees(void) {
  struct vector input = {fpt_fromint(10), fpt_fromint(10)};
  struct vector snapped = snap_to_axis(input, snap_threshold(fpt_fromint(45)));

  ASSERT_EQ(input.x, snapped.x);
  ASSERT_EQ(input.y, snapped.y);
  PASS();
}

TEST angle_snap_is_applied_after_rotation(void) {
  struct accel_args args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .input_dpi = fpt_fromint(1000),
      .angle_rotation_deg = fpt_fromint(-5),
      .angle_snap_threshold = snap_threshold(fpt_fromint(10)),
      .tag = no_accel,
  };
  int x = 100;
  int y = 10;

  f_accelerate(&x, &y, FIXEDPT_ONE, args);

  ASSERT(x >= 101);
  ASSERT_EQ(0, y);
  PASS();
}

TEST angle_snap_handles_large_vectors(void) {
  struct vector snapped = snap_to_axis(
      (struct vector){fpt_fromint(46341), fpt_fromint(1)},
      snap_threshold(fpt_fromint(10)));

  ASSERT(snapped.x > 0);
  ASSERT_EQ(0, snapped.y);

  fpt max_value = (fpt)(((fptu)~0) >> 1);
  snapped = snap_to_axis(
      (struct vector){fpt_fromint(2147483647), fpt_fromint(100000000)},
      snap_threshold(fpt_fromint(10)));
  ASSERT_EQ(max_value, snapped.x);
  ASSERT_EQ(0, snapped.y);
  PASS();
}

SUITE(acceleration) {
  RUN_TEST(linear_identity);
  RUN_TEST(linear_default);
  RUN_TEST(linear_low_sensitivity);
  RUN_TEST(linear_moderate_acceleration);
  RUN_TEST(linear_low_acceleration);
  RUN_TEST(linear_yx_ratio);
  RUN_TEST(natural_identity);
  RUN_TEST(natural_decay);
  RUN_TEST(natural_offset);
  RUN_TEST(natural_limit);
  RUN_TEST(synchronous_default);
  RUN_TEST(no_accel_identity);
  RUN_TEST(no_accel_yx_ratio);
  RUN_TEST(rotation_45_degrees);
  RUN_TEST(rotation_90_degrees);
  RUN_TEST(angle_snap_horizontal);
  RUN_TEST(angle_snap_vertical);
  RUN_TEST(angle_snap_leaves_movement_outside_threshold);
  RUN_TEST(angle_snap_leaves_diagonal_at_forty_five_degrees);
  RUN_TEST(angle_snap_is_applied_after_rotation);
  RUN_TEST(angle_snap_handles_large_vectors);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  TEST_MAIN_BEGIN();
  RUN_SUITE(acceleration);
  GREATEST_MAIN_END();
}
