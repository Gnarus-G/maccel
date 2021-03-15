// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include "config.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>

//Needed for kernel_fpu_begin/end
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    //Pre Kernel 5.0.0
    #include <asm/i387.h>
#else
    #include <asm/fpu/api.h>
#endif

static inline int Leet_round(float x)
{
    if (x >= 0) {
        return (int)(x + 0.5f);
    } else {
        return (int)(x - 0.5f);
    }
}

// What do we have here? Code from Quake 3, which is also GPL.
// https://en.wikipedia.org/wiki/Fast_inverse_square_root
// Copyright (C) 1999-2005 Id Software, Inc.
static inline float Q_sqrt(float number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
    
    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
    
    return 1 / y;
}

// Acceleration happens here
void accelerate(int* x, int* y){    
	float delta_x, delta_y, ms, rate;
	float accel_sens = SENSITIVITY;
	static float carry_x = 0.0f;
    static float carry_y = 0.0f;
	static float last_ms = 1.0f;
	static ktime_t last;
	ktime_t now;

    //We are going to use the FPU within the kernel. So we need to safely switch context during all FPU processing in order to not corrupt the userspace FPU state
    kernel_fpu_begin();
    
	//PreScale raw data from mouse
    delta_x = (*x) * PRE_SCALE_X;
    delta_y = (*y) * PRE_SCALE_Y;

	//Calculate frametime to derive mouse rate & speed
	now = ktime_get();
    ms = (now - last)/(1000*1000);
	last = now;
	if(ms < 1) ms = last_ms;    //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
	if(ms > 100) ms = 100;      //Original InterAccel has 200 here. RawAccel rounds to 100. So do we.
	last_ms = ms;
	
	//printk("MOUSE: %ld", (long) (1000*ms));

    rate = Q_sqrt(delta_x * delta_x + delta_y * delta_y);

	//Apply speedcap (is actually a "distance"-cap)
    if(SPEED_CAP != 0){
        if (rate >= SPEED_CAP) {
            delta_x *= SPEED_CAP / rate;
            delta_y *= SPEED_CAP / rate;
        }
    }

	//Calculate rate from travelled overall distance and add possible rate offsets
    rate /= ms;
    rate -= OFFSET;

	//Apply linear acceleration on the sensitivity if applicable and limit maximum value
    if(rate > 0){
        rate *= ACCELERATION;
        accel_sens += rate;
    }
    if(SENS_CAP > 0 && accel_sens >= SENS_CAP){
        accel_sens = SENS_CAP;
    }

	//Actually apply accelerated sensitivity, allow post-scaling and apply carry from previous round
    accel_sens /= SENSITIVITY;
    delta_x *= accel_sens;
    delta_y *= accel_sens;
    delta_x *= POST_SCALE_X;
    delta_y *= POST_SCALE_Y;
    delta_x += carry_x;
    delta_y += carry_y;

    //Cast back to ints
    *x = Leet_round(delta_x);
    *y = Leet_round(delta_y);

	//Save carry for next round
	carry_x = delta_x - *x;
    carry_y = delta_y - *y;
    
    //We stopped using the FPU: Switch back context again
    kernel_fpu_end();
}