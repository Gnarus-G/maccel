#include "dbg.h"
#include "linux/input.h"
#include <linux/stddef.h>

#define NONE_EVENT_VALUE 0

typedef struct {
  int *x;
  int *y;
} mouse_move;

static mouse_move MOVEMENT = {.x = NULL, .y = NULL};

/*
 * Track whether we injected synthetic storage for a missing axis.
 * When rotation is active and the mouse only reports one axis (e.g. pure
 * horizontal movement -> only REL_X), we need a place for f_accelerate
 * to write the rotated cross-axis component. These synthetic values
 * are later injected into the event stream by maccel_events().
 */
static bool injected_x = false;
static bool injected_y = false;
static int synthetic_x_val = 0;
static int synthetic_y_val = 0;

static inline void update_mouse_move(struct input_value *value) {
  switch (value->code) {
  case REL_X:
    MOVEMENT.x = &value->value;
    break;
  case REL_Y:
    MOVEMENT.y = &value->value;
    break;
  default:
    dbg("bad movement input_value: (code, value) = (%d, %d)", value->code,
        value->value);
  }
}

static inline int get_x(mouse_move movement) {
  if (movement.x == NULL) {
    return NONE_EVENT_VALUE;
  }
  return *movement.x;
}

static inline int get_y(mouse_move movement) {
  if (movement.y == NULL) {
    return NONE_EVENT_VALUE;
  }
  return *movement.y;
}

static inline void set_x_move(int value) {
  if (MOVEMENT.x == NULL) {
    return;
  }
  *MOVEMENT.x = value;
}

static inline void set_y_move(int value) {
  if (MOVEMENT.y == NULL) {
    return;
  }
  *MOVEMENT.y = value;
}

/*
 * When rotation is active and one axis is missing from the frame,
 * point the missing axis to synthetic storage so f_accelerate can
 * write the rotated component into it.
 */
static inline void ensure_axes_for_rotation(void) {
  if (MOVEMENT.x == NULL) {
    synthetic_x_val = 0;
    MOVEMENT.x = &synthetic_x_val;
    injected_x = true;
    dbg("rotation: injecting synthetic REL_X storage (x=%d)", 0);
  }
  if (MOVEMENT.y == NULL) {
    synthetic_y_val = 0;
    MOVEMENT.y = &synthetic_y_val;
    injected_y = true;
    dbg("rotation: injecting synthetic REL_Y storage (y=%d)", 0);
  }
}

static inline void clear_mouse_move(void) {
  MOVEMENT.x = NULL;
  MOVEMENT.y = NULL;
  injected_x = false;
  injected_y = false;
}
