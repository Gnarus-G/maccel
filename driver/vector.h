#ifndef _VECTOR_H_
#define _VECTOR_H_

#include "fixedptc.h"

struct vector {
  fixedpt x;
  fixedpt y;
};

static inline fixedpt magnitude(struct vector v) {
  fixedpt x_square = fixedpt_mul(v.x, v.x);
  fixedpt y_square = fixedpt_mul(v.y, v.y);
  fixedpt x_square_plus_y_square = fixedpt_add(x_square, y_square);

  dbg("dx^2 (in)                  %s", fptoa(x_square));
  dbg("dy^2 (in)                  %s", fptoa(y_square));
  dbg("square modulus (in)        %s", fptoa(x_square_plus_y_square));

  return fixedpt_sqrt(x_square_plus_y_square);
}

#endif // !_VECTOR_H_
