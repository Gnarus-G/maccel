#ifndef _ACCEL_H_
#define _ACCEL_H_

#include "dbg.h"
#include "fixedptc.h"

// fixedpt doesn't have a tanh function, so i made one
static inline fixedpt fixedpt_tanh(fixedpt x) {
  fixedpt numerator = fixedpt_sub(fixedpt_exp(x), fixedpt_exp(-x));
  fixedpt denominator = fixedpt_add(fixedpt_exp(x), fixedpt_exp(-x));
  return fixedpt_div(numerator, denominator);
}

static const fixedpt FIXEDPT_ZERO = fixedpt_rconst(0.0);

// Linear profile

static inline fixedpt linear_profile(fixedpt param_accel, fixedpt input_speed) {
  return fixedpt_add(FIXEDPT_ONE, fixedpt_mul((param_accel), input_speed));
}


// Logarithmic profile, didn't manage to test it
static inline fixedpt log_profile(fixedpt input_speed) {
  // Logarithmic curve (base can be adjusted)
  return fixedpt_ln(fixedpt_add(FIXEDPT_ONE, input_speed)); // log(1 + input_speed)
}

static inline fixedpt log_function(fixedpt input_speed) {
  return fixedpt_log(input_speed, fixedpt_rconst(10));
}

// Sigmoid profile, tested it, didn't behave as expetced
static inline fixedpt sigmoid_profile(fixedpt input_speed) {
  // Sigmoid curve: 1 / (1 + exp(-input_speed))
  fixedpt exp_input_speed = fixedpt_exp(input_speed);
  return fixedpt_div(FIXEDPT_ONE, fixedpt_add(FIXEDPT_ONE, exp_input_speed));
}

static inline fixedpt raw_accel_motivity(fixedpt input_speed) {
  if (input_speed == fixedpt_rconst(13)) {
    return FIXEDPT_ONE;
  }

  fixedpt log_motivity = fixedpt_log(fixedpt_rconst(2.1),fixedpt_rconst(10)); // use args.motivity
  fixedpt gamma_const = fixedpt_div(fixedpt_rconst(0.7), log_motivity); // use args.gamma aka growth rate
  fixedpt log_syncspeed = fixedpt_log(fixedpt_rconst(13),fixedpt_rconst(10)); // use args.sync_speed aka midpoint
  fixedpt syncspeed = fixedpt_rconst(13); // use args.sync_speed aka midpoint
  
  
  fixedpt sharpness = fixedpt_rconst(0.94);
  fixedpt sharpness_recip = fixedpt_div(FIXEDPT_ONE, sharpness);
  // fixedpt use_linear_clamp(sharpness >= 16);
  
  fixedpt minimum_sens = fixedpt_div(FIXEDPT_ONE, fixedpt_rconst(2.1));
  fixedpt maximum_sens = fixedpt_rconst(2.1);

  fixedpt log_x = fixedpt_log(input_speed,fixedpt_rconst(10));
  fixedpt log_diff = fixedpt_sub(log_x, log_syncspeed);
  
  if (log_diff > FIXEDPT_ZERO) {
  
    fixedpt log_space = fixedpt_mul(gamma_const, log_diff);
    fixedpt log_space_sharpened =  fixedpt_pow(log_space, sharpness);
    fixedpt tanh_log_space_sharpened = fixedpt_tanh(log_space_sharpened);

    fixedpt exponent = fixedpt_pow(tanh_log_space_sharpened, sharpness_recip);
    fixedpt result = fixedpt_exp(fixedpt_mul(exponent, log_motivity));

    dbg("raw accel motivity input_speed: %s", fixedpt_cstr(input_speed, 6));
    dbg("raw accel motivity log_x: %s", fixedpt_cstr(log_x, 6));
    dbg("raw accel motivity log_diff: %s", fixedpt_cstr(log_diff, 6));

    dbg("raw accel motivity log_motivity: %s", fixedpt_cstr(log_motivity, 6));
    dbg("raw accel motivity gamma_const: %s", fixedpt_cstr(gamma_const, 6));
    dbg("raw accel motivity log_space: %s", fixedpt_cstr(log_space, 6));

    dbg("raw accel motivity log_space_sharpened: %s", fixedpt_cstr(log_space_sharpened, 6));
    dbg("raw accel motivity tanh_log_space_sharpened: %s", fixedpt_cstr(tanh_log_space_sharpened, 6));
    dbg("raw accel motivity sharpness_recip: %s", fixedpt_cstr(sharpness_recip, 6));

    dbg("raw accel motivity exponent: %s", fixedpt_cstr(exponent, 6));
    dbg("raw accel motivity result: %s", fixedpt_cstr(result, 6));

    return result;
  } 

  else {
    fixedpt log_space = fixedpt_mul(fixedpt_mul(gamma_const, log_diff),fixedpt_rconst(-1));
    fixedpt exponent = fixedpt_mul(fixedpt_pow(fixedpt_tanh(fixedpt_pow(log_space, sharpness)), sharpness_recip),fixedpt_rconst(-1));
    fixedpt result = fixedpt_exp(fixedpt_mul(exponent, log_motivity));
    return result;
  } 

  return FIXEDPT_ONE;
}

// Kinda motivity, didn't manage to test it yet
static inline fixedpt alisk_motivity(fixedpt input_speed) {
    
    // Precompute values for clarity and debugging
    fixedpt max_sens_factor = fixedpt_rconst(2.71);
    fixedpt growth = fixedpt_rconst(-0.39);
    fixedpt shift_amount = fixedpt_rconst(6);

    fixedpt numerator = fixedpt_sub(max_sens_factor, FIXEDPT_ONE);
    
    // Inner exponential expression
    fixedpt exp_term = fixedpt_exp(fixedpt_add(fixedpt_mul(growth, input_speed), shift_amount));
    fixedpt denominator = fixedpt_add(FIXEDPT_ONE, exp_term);
    
    // Compute the motivity
    fixedpt result = fixedpt_add(FIXEDPT_ONE, fixedpt_div(numerator, denominator));
    return result;
    
    // oneliner = faster?
}

typedef unsigned int u32;

typedef struct {
  int x;
  int y;
} AccelResult;



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
  dbg("input_speed %s", fixedpt_cstr(input_speed, 6));

  if (input_speed > FIXEDPT_ZERO) {
    // Use linear acceleration
    // sens = linear_profile(param_accel, input_speed);

    // Use a logarithmic profile
    // sens = log_profile(input_speed);

    // Or use a sigmoid profile instead
    // sens = sigmoid_profile(input_speed);

    // Use motivity like function 1+ ((e-1)/(1+e^-(0.4*x-6))) (see: https://www.desmos.com/calculator  input: y=\frac{e-1}{1+e^{-\left(0.4x-6\right)}}+1)
    dbg("motivity input speed    %s", fixedpt_cstr(input_speed, 6));
    sens = raw_accel_motivity(input_speed);
    // sens = fixedpt_mul(input_speed, input_speed);
    // dbg("motivity yeah");
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
