#include "dbg.h"
#include "linux/input.h"
#include <linux/stddef.h>

#define NONE_EVENT_VALUE 0

typedef struct {
  int *x;
  int *y;
} mouse_move;

static mouse_move MOVEMENT = {.x = NULL, .y = NULL};

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

static inline void clear_mouse_move(void) {
  MOVEMENT.x = NULL;
  MOVEMENT.y = NULL;
}
