#include "../accel.h"
#include "test_utils.h"

static int fpt_near(fpt actual, fpt expected, fpt tolerance) {
  return fpt_abs(actual - expected) <= tolerance;
}

TEST matches_raw_accel_reference(void) {
  struct synchronous_gain_curve_args args = {
      .gamma = fpt_rconst(0.8),
      .smooth = fpt_rconst(0.5),
      .motivity = fpt_rconst(1.5),
      .sync_speed = fpt_rconst(32),
  };
  struct synchronous_gain_lut lut;
#if FIXEDPT_BITS == 64
  const fpt tolerance = fpt_rconst(0.0002);
#else
  const fpt tolerance = fpt_rconst(0.002);
#endif

  __synchronous_gain_lut_init(&lut, args);

  ASSERT(fpt_near(__synchronous_gain_lut_lookup(fpt_rconst(0.125), &lut),
                  fpt_rconst(0.6666666668), tolerance));
  ASSERT(fpt_near(__synchronous_gain_lut_lookup(fpt_rconst(5), &lut),
                  fpt_rconst(0.6667458263), tolerance));
  ASSERT(fpt_near(__synchronous_gain_lut_lookup(fpt_rconst(32), &lut),
                  fpt_rconst(0.7544606092), tolerance));
  ASSERT(fpt_near(__synchronous_gain_lut_lookup(fpt_rconst(512), &lut),
                  fpt_rconst(1.4372989449), tolerance));
  ASSERT(fpt_near(__synchronous_gain_lut_lookup(fpt_rconst(1024), &lut),
                  fpt_rconst(1.4686379769), tolerance));
  PASS();
}

TEST clamps_sensitivity_below_first_point(void) {
  struct synchronous_gain_curve_args args = {
      .gamma = fpt_rconst(0.8),
      .smooth = fpt_rconst(0.5),
      .motivity = fpt_rconst(1.5),
      .sync_speed = fpt_rconst(32),
  };
  struct synchronous_gain_lut lut;

  __synchronous_gain_lut_init(&lut, args);

  ASSERT_EQ(__synchronous_gain_lut_lookup(fpt_rconst(0.125), &lut),
            __synchronous_gain_lut_lookup(fpt_rconst(0.01), &lut));
  PASS();
}

TEST is_a_separate_acceleration_mode(void) {
  struct synchronous_gain_curve_args gain_curve = {
      .gamma = fpt_rconst(0.8),
      .smooth = fpt_rconst(0.5),
      .motivity = fpt_rconst(1.5),
      .sync_speed = fpt_rconst(32),
  };
  struct accel_args gain_args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .tag = synchronous_gain,
      .args = (union __accel_args){.synchronous_gain = gain_curve},
  };
  struct accel_args legacy_args = {
      .sens_mult = FIXEDPT_ONE,
      .yx_ratio = FIXEDPT_ONE,
      .tag = synchronous,
      .args =
          (union __accel_args){
              .synchronous =
                  {
                      .gamma = fpt_rconst(0.8),
                      .smooth = fpt_rconst(0.5),
                      .motivity = fpt_rconst(1.5),
                      .sync_speed = fpt_rconst(32),
                  },
          },
  };
  struct vector gain = sensitivity(fpt_rconst(32), gain_args);
  struct vector legacy = sensitivity(fpt_rconst(32), legacy_args);

  ASSERT(fpt_near(gain.x, fpt_rconst(0.7544606092), fpt_rconst(0.0002)));
  ASSERT_EQ(FIXEDPT_ONE, legacy.x);
  PASS();
}

SUITE(synchronous_gain_tests) {
  RUN_TEST(matches_raw_accel_reference);
  RUN_TEST(clamps_sensitivity_below_first_point);
  RUN_TEST(is_a_separate_acceleration_mode);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(synchronous_gain_tests);
  GREATEST_MAIN_END();
}
