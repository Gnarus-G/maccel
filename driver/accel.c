// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include "util.h"
#include "float.h"
#include "config.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/string.h>   //strlen

//Needed for kernel_fpu_begin/end
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    //Pre Kernel 5.0.0
    #include <asm/i387.h>
#else
    #include <asm/fpu/api.h>
#endif

MODULE_AUTHOR("Christopher Williams <chilliams (at) gmail (dot) com>"); //Original idea of this module
MODULE_AUTHOR("Klaus Zipfel <klaus (at) zipfel (dot) family>");         //Current maintainer

//Converts a preprocessor define's value in "config.h" to a string - Suspect this to change in future version without a "config.h"
#define _s(x) #x
#define s(x) _s(x)

//Convenient helper for float based parameters, which are passed via a string to this module (must be individually parsed via atof() - available in util.c)
#define PARAM_F(param, default, desc)                           \
    float g_##param = default;                                  \
    static char* g_param_##param = s(default);                  \
    module_param_named(param, g_param_##param, charp, 0644);    \
    MODULE_PARM_DESC(param, desc);

#define PARAM(param, default, desc)                             \
    static char g_##param = default;                            \
    module_param_named(param, g_##param, byte, 0644);           \
    MODULE_PARM_DESC(param, desc);

// ########## Kernel module parameters

// Simple module parameters (instant update)
PARAM(no_bind,          0,                  "This will disable binding to this driver via 'leetmouse_bind' by udev.");
PARAM(update,           0,                  "Triggers an update of the acceleration parameters below");
PARAM(AccelerationMode, ACCELERATION_MODE,  "Sets the algorithm to be used for acceleration");

//PARAM(AccelMode,        MODE,           "Acceleration method: 0 power law, 1: saturation, 2: log"); //Not yet implemented

// Acceleration parameters (type pchar. Converted to float via "updata_params" triggered by /sys/module/leetmouse/parameters/update)
PARAM_F(PreScaleX,      PRE_SCALE_X,        "Prescale X-Axis before applying acceleration.");
PARAM_F(PreScaleY,      PRE_SCALE_Y,        "Prescale Y-Axis before applying acceleration.");
PARAM_F(SpeedCap,       SPEED_CAP,          "Limit the maximum pointer speed before applying acceleration.");
PARAM_F(Sensitivity,    SENSITIVITY,        "Mouse base sensitivity.");
PARAM_F(Acceleration,   ACCELERATION,       "Mouse acceleration sensitivity.");
PARAM_F(SensitivityCap, SENS_CAP,           "Cap maximum sensitivity.");
PARAM_F(Offset,         OFFSET,             "Mouse base sensitivity.");
PARAM_F(Exponent,       EXPONENT,           "Exponent for algorithms that use it");           //Not yet implemented
PARAM_F(PostScaleX,     POST_SCALE_X,       "Postscale X-Axis after applying acceleration.");
PARAM_F(PostScaleY,     POST_SCALE_Y,       "Postscale >-Axis after applying acceleration.");
//PARAM_F(AngleAdjustment,XXX,            "");           //Not yet implemented. Douptful, if I will ever add it - Not very useful and needs me to implement trigonometric functions from scratch in C.
//PARAM_F(AngleSnapping,  XXX,            "");           //Not yet implemented. Douptful, if I will ever add it - Not very useful and needs me to implement trigonometric functions from scratch in C.
PARAM_F(ScrollsPerTick, SCROLLS_PER_TICK,   "Amount of lines to scroll per scroll-wheel tick.");


// Updates the acceleration parameters. This is purposely done with a delay!
// First, to not hammer too much the logic in "accelerate()", which is called VERY OFTEN!
// Second, to fight possible cheating. However, this can be OFC changed, since we are OSS...
#define PARAM_UPDATE(param) atof(g_param_##param, strlen(g_param_##param) , &g_##param);

static ktime_t g_next_update = 0;
INLINE void updata_params(ktime_t now)
{
    if(!g_update) return;
    if(now < g_next_update) return;
    g_update = 0;
    g_next_update = now + 1000000000ll;    //Next update is allowed after 1s of delay

    PARAM_UPDATE(PreScaleX);
    PARAM_UPDATE(PreScaleY);
    PARAM_UPDATE(SpeedCap);
    PARAM_UPDATE(Sensitivity);
    PARAM_UPDATE(Acceleration);
    PARAM_UPDATE(SensitivityCap);
    PARAM_UPDATE(Offset);
    PARAM_UPDATE(PostScaleX);
    PARAM_UPDATE(PostScaleY);
    PARAM_UPDATE(ScrollsPerTick);
    PARAM_UPDATE(Exponent);
}

// ########## Acceleration code

// Acceleration happens here
int accelerate(int *x, int *y, int *wheel)
{
	float delta_x, delta_y, delta_whl, ms, rate, accel_sens, product;
    static long buffer_x = 0;
    static long buffer_y = 0;
    static long buffer_whl = 0;
    //Static float assignment should happen at compile-time and thus should be safe here. However, avoid non-static assignment of floats outside kernel_fpu_begin()/kernel_fpu_end()
	static float carry_x = 0.0f;
    static float carry_y = 0.0f;
    static float carry_whl = 0.0f;
	static float last_ms = 1.0f;
	static ktime_t last;
	ktime_t now;
    int status = 0;

    // We can only safely use the FPU in an IRQ event when this returns 1.
    // Not taking care for this interfered with BTRFS on my machine (which also uses kernel_fpu_begin/kernel_fpu_end) and lead to data corruption. And I guess, the same would be true for raid6 (both use kernel_fpu_begin/kernel_fpu_end).
    if(!irq_fpu_usable()){
        // Buffer mouse deltas for next (valid) IRQ
        buffer_x += *x;
        buffer_y += *y;
        buffer_whl += *wheel;
        return -EBUSY;
    }

//We are going to use the FPU within the kernel. So we need to safely switch context during all FPU processing in order to not corrupt the userspace FPU state
//Note: Avoid any function calls (https://yarchive.net/comp/linux/kernel_fp.html - Torvalds: "It all has to be stuff that gcc can do in-line,without any function calls.")
//This is why we use the "INLINE" pre-processor directive (defined in util.h), which expands to "__attribute__((always_inline)) inline" in order to force gcc to inline the functions defined in float.h
//Not doing this caused the FPU state to get randomly screwed up (https://github.com/systemofapwne/leetmouse/issues/4), making the cursor to get stuck on the left screen. Especially when playing certain videos in the browser.
kernel_fpu_begin();
    accel_sens = g_Sensitivity;

    delta_x = (float) (*x);
    delta_y = (float) (*y);
    delta_whl = (float) (*wheel);

    // When compiled with mhard-float, I noticed that casting to float sometimes returns invalid values, especially when playing this video in brave/chrome/chromium
    // https://sps-tutorial.com/was-ist-eine-sps/ or https://www.youtube.com/watch?v=tjT9gt0dArQ or https://www.ginx.tv/en/cs-go/cs-go-trusted-mode-how-to-enable-third-party-software
    // Here we check, if casting did work out.
    if(!((int) delta_x == *x && (int) delta_y == *y && (int) delta_whl == *wheel)){
        // Buffer mouse deltas for next (valid) IRQ
        buffer_x += *x;
        buffer_y += *y;
        buffer_whl += *wheel;
        // Jump out of kernel_fpu_begin
        status = -EFAULT;
        printk("LEETMOUSE: First float-trap triggered. Should very very rarely happen, if at all");
        goto exit;
    }

    //Add buffer values, if present, and reset buffer
    delta_x += (float) buffer_x; buffer_x = 0;
    delta_y += (float) buffer_y; buffer_y = 0;
    delta_whl += (float) buffer_whl; buffer_whl = 0;

    //Calculate frametime
    now = ktime_get();
    ms = (now - last)/(1000*1000);
    last = now;
    if(ms < 1) ms = last_ms;    //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
    if(ms > 100) ms = 100;      //Original InterAccel has 200 here. RawAccel rounds to 100. So do we.
    last_ms = ms;

    //Update acceleration parameters periodically
    updata_params(now);

    //Prescale
    delta_x *= g_PreScaleX;
    delta_y *= g_PreScaleY;

    //Calculate velocity (one step before rate, which divides rate by the last frametime)
    rate = delta_x * delta_x + delta_y * delta_y;
    B_sqrt(&rate);

    //Apply speedcap
    if(g_SpeedCap != 0){
        if (rate >= g_SpeedCap) {
            delta_x *= g_SpeedCap / rate;
            delta_y *= g_SpeedCap / rate;
            rate = g_SpeedCap;
        }
    }

    //Calculate rate from travelled overall distance and add possible rate offsets
    rate /= ms;
    rate -= g_Offset;

    switch(g_AccelerationMode){

        //Linear Acceleration
        case 1 : 
            if(rate > 0){
                rate *= g_Acceleration;
                accel_sens += rate;
                
            }
            break;
        
        //Classic Acceleration
        case 2 :
            if(rate > 0){
                product = rate * g_Acceleration;
                B_pow(&product, &g_Exponent);
                accel_sens += product;
            }
            break;

    }
    
    if(g_SensitivityCap > 0 && accel_sens >= g_SensitivityCap){
        accel_sens = g_SensitivityCap;
    }

    //Actually apply accelerated sensitivity, allow post-scaling and apply carry from previous round
    accel_sens /= g_Sensitivity;
    delta_x *= accel_sens;
    delta_y *= accel_sens;
    delta_x *= g_PostScaleX;
    delta_y *= g_PostScaleY;
    delta_whl *= g_ScrollsPerTick/3.0f;
    delta_x += carry_x;
    delta_y += carry_y;
    if((delta_whl < 0 && carry_whl < 0) || (delta_whl > 0 && carry_whl > 0)) //Only apply carry to the wheel, if it shares the same sign
        delta_whl += carry_whl;

    //Last check for validity
    if(!(isfinite(&delta_x) && isfinite(&delta_y) && isfinite(&delta_whl))){
        // Buffer mouse deltas for next (valid) IRQ
        buffer_x += *x;
        buffer_y += *y;
        buffer_whl += *wheel;
        // Jump out of kernel_fpu_begin
        printk("LEETMOUSE: Acceleration of NaN value");
        status = -EFAULT;
        goto exit;
    }

    //Cast back to int
    *x = Leet_round(&delta_x);
    *y = Leet_round(&delta_y);
    *wheel = Leet_round(&delta_whl);

    // Very last trap. This should NEVER get triggered. Buf if the FPU state gets screwed up "somehow", it seems like the floats get casted to MIN_INT (-2147483648). So we trap this edge case
    if(*x == -2147483648 || *y == -2147483648 || *wheel == -2147483648){
        // Jump out of kernel_fpu_begin
        printk("LEETMOUSE: Final float-trap triggered. This should NEVER happen!");
        status = -EFAULT;
        goto exit;
    }

    //Save carry for next round
    carry_x = delta_x - *x;
    carry_y = delta_y - *y;
    carry_whl = delta_whl - *wheel;
    
exit:
//We stopped using the FPU: Switch back context again
kernel_fpu_end();

    return status;
}
