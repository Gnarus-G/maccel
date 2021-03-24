// Maximum number of packets allowed to be sent from the mouse at once. Linux's default value is 8, which at
// least causes EOVERFLOW for my mouse (SteelSeries Rival 600). Increase this, if 'dmesg -w' tells you to!
#define BUFFER_SIZE 16

/*
 * This should be your desired acceleration. It needs to end with an f.
 * For example, setting this to "0.1f" should be equal to
 * cl_mouseaccel 0.1 in Quake.
 */

// Changes behaviour of the scroll-wheel. Default is 3.0f
#define SCROLLS_PER_TICK 5.0f

// Emulate Windows' "Enhanced Pointer Precision" for my mouse (1000 Hz) by approximating it with a linear accel
#define SENSITIVITY 0.85f
#define ACCELERATION 0.26f
#define SENS_CAP 4.0f
#define OFFSET 0.0f
#define POST_SCALE_X 0.4f
#define POST_SCALE_Y 0.4f
#define SPEED_CAP 0.0f

// Prescaler for different DPI values. 1.0f at 400 DPI. To adjust it for <your_DPI>, calculate 400/your_DPI

// Generic @ 400 DPI
#define PRE_SCALE_X 1.0f
#define PRE_SCALE_Y 1.0f

// Steelseries Rival 110 @ 7200 DPI
//#define PRE_SCALE_X 0.0555555f
//define PRE_SCALE_Y 0.0555555f

// Steelseries Rival 600/650 @ 12000 DPI
//#define PRE_SCALE_X 0.0333333f
//#define PRE_SCALE_Y 0.0333333f
