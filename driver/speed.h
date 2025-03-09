#ifndef __SPEED_H__
#define __SPEED_H__

#include "dbg.h"
#include "fixedptc.h"
#include "math.h"

/**
 * Track this to enable the UI to show the last noted
 * input counts/ms (speed).
 */
static fpt LAST_INPUT_MOUSE_SPEED = 0;

static inline fpt input_speed(fpt dx, fpt dy, fpt time_ms) {

  fpt distance = magnitude((struct vector){dx, dy});

  if (distance == -1) {
    dbg("distance calculation failed: t = %s", fptoa(time_ms));
    return 0;
  }

  dbg("distance (in)              %s", fptoa(distance));

  fpt speed = fpt_div(distance, time_ms);
  LAST_INPUT_MOUSE_SPEED = speed;

  dbg("time interval              %s", fptoa(time_ms));
  dbg("speed (in)                 %s", fptoa(speed));

  return speed;
}

#endif // !__SPEED_H__
