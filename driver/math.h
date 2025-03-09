#ifndef _MATH_H_
#define _MATH_H_

#include "fixedptc.h"

struct vector {
  fpt x;
  fpt y;
};

static inline fpt magnitude(struct vector v) {
  fpt x_square = fpt_mul(v.x, v.x);
  fpt y_square = fpt_mul(v.y, v.y);
  fpt x_square_plus_y_square = fpt_add(x_square, y_square);

  dbg("dx^2 (in)                  %s", fptoa(x_square));
  dbg("dy^2 (in)                  %s", fptoa(y_square));
  dbg("square modulus (in)        %s", fptoa(x_square_plus_y_square));

  return fpt_sqrt(x_square_plus_y_square);
}

static inline fpt minsd(fpt a, fpt b) { return (a < b) ? a : b; }

#endif // !_MATH_H_
