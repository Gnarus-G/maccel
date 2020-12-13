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

// ########################################
// IMPORTANT:   All values below must be INTEGERS (we want to avoid floating point arithmetic in the kernel)
//              In order to not give up precision, just multiply your InterAccell values by 1000
//              E.g: If your InterAccell Acceleration value is 0.26, write ACCELERATION 260
//
//              I know that this is unconvenient. But this makes sure, that the kernel module works on all machines
// ########################################

#define ACCELERATION 260

#define SENSITIVITY 850
#define SENS_CAP 4000
#define OFFSET 0

// Steelseries Rival 110 @ 7200 DPI
#define PRE_SCALE_X 55
#define PRE_SCALE_Y 55

// Steelseries Rival 600/610 @ 12000 DPI
//#define PRE_SCALE_X 33
//#define PRE_SCALE_Y 33

#define POST_SCALE_X 400
#define POST_SCALE_Y 400
#define SPEED_CAP 0
