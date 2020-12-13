/*
* Your mouse's polling rate.
* If you don't know what yours is, follow this link:
* https://wiki.archlinux.org/index.php/Mouse_polling_rate
*/
//TODO: Actually derive "polling rate" live from mouse input events, as it is done in InterAcceel.
#define POLLING_RATE 1000

/*
 * This should be your desired acceleration. It needs to end with an f.
 * For example, setting this to "0.1f" should be equal to
 * cl_mouseaccel 0.1 in Quake.
 */

// These settings basically emulate Windows' Enhanced Pointer Precision for my 7200 DPI mouse

#define ACCELERATION 0.27f

#define SENSITIVITY 0.85f
#define SENS_CAP 4.0f
#define OFFSET 0.5f

// Steelseries Rival 110 @ 7200 DPI
//#define PRE_SCALE_X 0.0555555f
//define PRE_SCALE_Y 0.0555555f

// Steelseries Rival 600/610 @ 12000 DPI
#define PRE_SCALE_X 0.0333333f
#define PRE_SCALE_Y 0.0333333f

#define POST_SCALE_X 0.4f
#define POST_SCALE_Y 0.4f
#define SPEED_CAP 0.0f
