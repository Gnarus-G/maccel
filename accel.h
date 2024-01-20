#include "asm-generic/int-ll64.h"

#define ACCEL_FACTOR 0.3

#define INLINE __attribute__((always_inline)) inline

static INLINE float Q_sqrt(float number) {
  long i;
  float x2, y;
  const float threehalfs = 1.5F;

  x2 = number * 0.5F;
  y = number;
  i = *(long *)&y;           // evil floating point bit level hacking
  i = 0x5f3759df - (i >> 1); // what the fuck?
  y = *(float *)&i;
  y = y * (threehalfs - (x2 * y * y)); // 1st iteration
  //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can
  // be removed

  return 1 / y;
}

typedef struct {
  s8 x;
  s8 y;
} AccelResult;

AccelResult INLINE accelerate(s8 x, s8 y, u32 polling_interval) {
  AccelResult result = {};
  float dx = x;
  float dy = y;

  /* printk(KERN_INFO "[MOUSE_MOVE] (%d, %d)", (int)dx, (int)dy); */

  float distance = Q_sqrt(dx * dx + dy * dy);
  float speed_in = distance / polling_interval;

  float speed_factor = (1 + ACCEL_FACTOR * speed_in);
  float dx_out = dx * speed_factor;
  float dy_out = dy * speed_factor;

  result.x = (s8)dx_out;
  result.y = (s8)dy_out;

  /* printk(KERN_INFO "[MOUSE_MOVE_ACCEL] (%d, %d)", (int)dx, (int)dy); */

  return result;
}
