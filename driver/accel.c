// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include "util.h"
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


//Converts a preprocessor define's value in "config.h" to a string - Suspect this to change in future version without a "config.h"
#define _s(x) #x
#define s(x) _s(x)

#define PARAM(param, default, desc)                             \
    float g_##param = default;                                  \
    static char* g_param_##param = s(default);                  \
    module_param_named(param, g_param_##param, charp, 0644);    \
    MODULE_PARM_DESC(param, desc);

// ########## Kernel module parameters
PARAM(no_bind, 0, "This will disable binding to this driver via 'leetmouse_bind' by udev.")

PARAM(sensitivity, SENSITIVITY, "Mouse base sensitivity")

// Updates the acceleration parameters. This is purposely done with a delay!
// First, to not hammer too much the logic in "accelerate()", which is called VERY OFTEN!
// Second, to fight possible cheating. However, this can be OFC changed, since we are OSS...
static void updata_params(void)
{
    return;
}

// ########## Acceleration code

// Acceleration happens here
int accelerate(int *x, int *y, int *wheel)
{
	float delta_x, delta_y, delta_whl, ms, rate;
	float accel_sens = SENSITIVITY;
    static long buffer_x = 0;
    static long buffer_y = 0;
    static long buffer_whl = 0;
	static float carry_x = 0.0f;
    static float carry_y = 0.0f;
    static float carry_whl = 0.0f;
	static float last_ms = 1.0f;
	static ktime_t last;
	ktime_t now;
    int status = 0;

    // We can only safely use the FPU in an IRQ event when this returns 1.
    // This is especially important, when compiling this module with msse (triggered it a lot) instead of mhard-float (never triggered it for me)
    // Not taking care for this lead to data-corruption of my BTRFS volumes. And I guess, the same would be true for raid6 (both use kernel_fpu_begin/kernel_fpu_end).
    if(!irq_fpu_usable()){
        // Buffer mouse deltas for next (valid) IRQ
        buffer_x += *x;
        buffer_y += *y;
        buffer_whl += *wheel;
        return 1;
    }

//We are going to use the FPU within the kernel. So we need to safely switch context during all FPU processing in order to not corrupt the userspace FPU state
kernel_fpu_begin();

    delta_x = (float) (*x);
    delta_y = (float) (*y);
    delta_whl = (float) (*wheel);

    // When compiled with mhard-float, I noticed that casting to float sometimes returns invalid values, especially when playing this video in brave/chrome/chromium
    // https://sps-tutorial.com/was-ist-eine-sps/
    // Here we check, if the casting did work out.
    if(!((int) delta_x == *x || (int) delta_y == *y || (int) delta_whl == *wheel)){
        // Buffer mouse deltas for next (valid) IRQ
        buffer_x += *x;
        buffer_y += *y;
        buffer_whl += *wheel;
        // Jump out of kernel_fpu_begin
        status = 1;
        goto exit;
    }

    //Add buffer values, if present, and reset buffer
    delta_x += (float) buffer_x; buffer_x = 0;
    delta_y += (float) buffer_y; buffer_y = 0;
    delta_whl += (float) buffer_whl; buffer_whl = 0;

    //Update acceleration parameters periodically
    updata_params();

    //Prescale
    delta_x *= PRE_SCALE_X;
    delta_y *= PRE_SCALE_Y;

    //Calculate frametime to derive mouse rate & speed
    now = ktime_get();
    ms = (now - last)/(1000*1000);
    last = now;
    if(ms < 1) ms = last_ms;    //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
    if(ms > 100) ms = 100;      //Original InterAccel has 200 here. RawAccel rounds to 100. So do we.
    last_ms = ms;

    rate = delta_x * delta_x + delta_y * delta_y;
    Q_sqrt(&rate);

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
    delta_whl *= SCROLLS_PER_TICK/3.0f;
    delta_x += carry_x;
    delta_y += carry_y;
    if((delta_whl < 0 && carry_whl < 0) || (delta_whl > 0 && carry_whl > 0)) //Only apply carry to the wheel, if it shares the same sign
        delta_whl += carry_whl;

    //Cast back to int
    *x = Leet_round(delta_x);
    *y = Leet_round(delta_y);
    *wheel = Leet_round(delta_whl);

    //Save carry for next round
    carry_x = delta_x - *x;
    carry_y = delta_y - *y;
    carry_whl = delta_whl - *wheel;
    
exit:
//We stopped using the FPU: Switch back context again
kernel_fpu_end();

    return status;
}
