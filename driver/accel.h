#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "dbg.h"
#include "fixedptc.h"

// Linear profile

static inline fixedpt linear_profile(fixedpt param_accel, fixedpt input_speed) {
  return fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
}


// Logarithmic profile, didn't manage to test it
static inline fixedpt log_profile(fixedpt input_speed) {
  // Logarithmic curve (base can be adjusted)
  return fixedpt_ln(fixedpt_add(FIXEDPT_ONE, input_speed)); // log(1 + input_speed)
}

// Sigmoid profile, tested it, didn't behave as expetced
static inline fixedpt sigmoid_profile(fixedpt input_speed) {
  // Sigmoid curve: 1 / (1 + exp(-input_speed))
  fixedpt exp_input_speed = fixedpt_exp(input_speed);
  return fixedpt_div(FIXEDPT_ONE, fixedpt_add(FIXEDPT_ONE, exp_input_speed));
}


// Kinda motivity, didn't manage to test it yet
static inline fixedpt motivity(fixedpt input_speed) {
    // Precompute values for clarity and debugging
    fixedpt exp_1 = fixedpt_exp(FIXEDPT_ONE);
    fixedpt numerator = fixedpt_sub(exp_1, FIXEDPT_ONE);
    
    // Inner exponential expression  -  todo: fix the following line
    // fixedpt exp_term = fixedpt_exp(fixedpt_sub(fixedpt_mul(FIXEDPT_CONST(-0.4), input_speed), FIXEDPT_CONST(-6.0)));

    fixedpt x = fixedpt_rconst(-0.4);
    fixedpt y = fixedpt_rconst(-6);

    fixedpt exp_term = fixedpt_exp(fixedpt_sub(fixedpt_mul(x, input_speed), y));

    // And delete this line!
    // fixedpt exp_term = fixedpt_exp(FIXEDPT_ONE);
    fixedpt denominator = fixedpt_add(FIXEDPT_ONE, exp_term);
    
    // Compute the motivity
    fixedpt result = fixedpt_add(FIXEDPT_ONE, fixedpt_div(numerator, denominator));
    
    return result;
}

typedef unsigned int u32;

typedef struct {
  int x;
  int y;
} AccelResult;

static const fixedpt FIXEDPT_ZERO = fixedpt_rconst(0.0);

/**
 * Calculate the factor by which to multiply the input vector
 * in order to get the desired output speed.
 *
 */
extern inline fixedpt sensitivity(fixedpt input_speed, fixedpt param_sens_mult,
                                  fixedpt param_accel, fixedpt param_offset,
                                  fixedpt param_output_cap) {

  input_speed = fixedpt_sub(input_speed, param_offset);

  fixedpt sens = FIXEDPT_ONE;

  dbg("accel    %s", fixedpt_cstr(param_accel, 6));

  if (input_speed > FIXEDPT_ZERO) {
    // Use linear acceleration
    // sens = linear_profile(param_accel, input_speed);

    // Use a logarithmic profile
    // sens = log_profile(input_speed);

    // Or use a sigmoid profile instead
    // sens = sigmoid_profile(input_speed);

    // Use motivity like function 1+ ((e-1)/(1+e^-(0.4*x-6))) (see: https://www.desmos.com/calculator  input: y=\frac{e-1}{1+e^{-\left(0.4x-6\right)}}+1)
    sens = motivity(input_speed);
  }

  sens = fixedpt_mul(sens, param_sens_mult);

  fixedpt output_cap = fixedpt_mul(param_output_cap, param_sens_mult);

  dbg("sens     %s", fixedpt_cstr(sens, 6));
  dbg("sens cap %s", fixedpt_cstr(output_cap, 6));

  if (param_output_cap != FIXEDPT_ZERO && sens > output_cap) {
    return output_cap;
  }

  return sens;
}

static inline fixedpt input_speed(fixedpt dx, fixedpt dy,
                                  u32 polling_interval) {
  fixedpt distance =
      fixedpt_sqrt(fixedpt_add(fixedpt_mul(dx, dx), fixedpt_mul(dy, dy)));

  dbg("distance (in)              %s", fixedpt_cstr(distance, 6));

  fixedpt speed_in = fixedpt_div(distance, fixedpt_fromint(polling_interval));

  dbg("polling interval           %s",
      fixedpt_cstr(fixedpt_fromint(polling_interval), 6));
  dbg("speed (in)                 %s", fixedpt_cstr(speed_in, 6));

  return speed_in;
}

static inline AccelResult f_accelerate(int x, int y, u32 polling_interval,
                                       fixedpt param_sens_mult,
                                       fixedpt param_accel,
                                       fixedpt param_offset,
                                       fixedpt param_output_cap) {
  AccelResult result = {.x = 0, .y = 0};

  static fixedpt carry_x = FIXEDPT_ZERO;
  static fixedpt carry_y = FIXEDPT_ZERO;

  fixedpt dx = fixedpt_fromint(x);
  fixedpt dy = fixedpt_fromint(y);

  dbg("in                        (%d, %d)", x, y);
  dbg("in: x (fixedpt conversion) %s", fixedpt_cstr(dx, 6));
  dbg("in: y (fixedpt conversion) %s", fixedpt_cstr(dy, 6));

  fixedpt speed_in = input_speed(dx, dy, polling_interval);
  fixedpt sens = sensitivity(speed_in, param_sens_mult, param_accel,
                             param_offset, param_output_cap);

  dbg("sens %s", fixedpt_cstr(sens, 6));

  fixedpt dx_out = fixedpt_mul(dx, sens);
  fixedpt dy_out = fixedpt_mul(dy, sens);

  dx_out = fixedpt_add(dx_out, carry_x);
  dy_out = fixedpt_add(dy_out, carry_y);

  dbg("out: x                     %s", fixedpt_cstr(dx_out, 6));
  dbg("out: y                     %s", fixedpt_cstr(dy_out, 6));

  result.x = fixedpt_toint(dx_out);
  result.y = fixedpt_toint(dy_out);

  dbg("out (int conversion)      (%d, %d)", result.x, result.y);

  carry_x = fixedpt_sub(dx_out, fixedpt_fromint(result.x));
  carry_y = fixedpt_sub(dy_out, fixedpt_fromint(result.y));

  dbg("carry                     (%s, %s)", fixedpt_cstr(carry_x, 6),
      fixedpt_cstr(carry_x, 6));

  return result;
}

#endif
