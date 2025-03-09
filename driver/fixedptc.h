#ifndef _FIXEDPTC_H_
#define _FIXEDPTC_H_

/*
 * fixedptc.h is a 32-bit or 64-bit fixed point numeric library.
 *
 * The symbol FIXEDPT_BITS, if defined before this library header file
 * is included, determines the number of bits in the data type (its "width").
 * The default width is 32-bit (FIXEDPT_BITS=32) and it can be used
 * on any recent C99 compiler. The 64-bit precision (FIXEDPT_BITS=64) is
 * available on compilers which implement 128-bit "long long" types. This
 * precision has been tested on GCC 4.2+.
 *
 * The FIXEDPT_WBITS symbols governs how many bits are dedicated to the
 * "whole" part of the number (to the left of the decimal point). The larger
 * this width is, the larger the numbers which can be stored in the fixedpt
 * number. The rest of the bits (available in the FIXEDPT_FBITS symbol) are
 * dedicated to the fraction part of the number (to the right of the decimal
 * point).
 *
 * Since the number of bits in both cases is relatively low, many complex
 * functions (more complex than div & mul) take a large hit on the precision
 * of the end result because errors in precision accumulate.
 * This loss of precision can be lessened by increasing the number of
 * bits dedicated to the fraction part, but at the loss of range.
 *
 * Adventurous users might utilize this library to build two data types:
 * one which has the range, and one which has the precision, and carefully
 * convert between them (including adding two number of each type to produce
 * a simulated type with a larger range and precision).
 *
 * The ideas and algorithms have been cherry-picked from a large number
 * of previous implementations available on the Internet.
 * Tim Hartrick has contributed cleanup and 64-bit support patches.
 *
 * == Special notes for the 32-bit precision ==
 * Signed 32-bit fixed point numeric library for the 24.8 format.
 * The specific limits are -8388608.999... to 8388607.999... and the
 * most precise number is 0.00390625. In practice, you should not count
 * on working with numbers larger than a million or to the precision
 * of more than 2 decimal places. Make peace with the fact that PI
 * is 3.14 here. :)
 */

/*-
 * Copyright (c) 2010-2012 Ivan Voras <ivoras@freebsd.org>
 * Copyright (c) 2012 Tim Hartrick <tim@edgecast.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "utils.h"

#ifndef FIXEDPT_BITS
#define FIXEDPT_BITS 64
#endif

#ifdef __KERNEL__
#include <linux/math64.h>
#include <linux/stddef.h>
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#if FIXEDPT_BITS == 32
typedef int32_t fpt;
typedef int64_t fptd;
typedef uint32_t fptu;
typedef uint64_t fptud;

#define FIXEDPT_WBITS 16

#elif FIXEDPT_BITS == 64
#include "Fixed64.utils.h"

typedef int64_t fpt;
typedef __int128_t fptd;
typedef uint64_t fptu;
typedef __uint128_t fptud;

#define FIXEDPT_WBITS 32
#else
#error "FIXEDPT_BITS must be equal to 32 or 64"
#endif

#if FIXEDPT_WBITS >= FIXEDPT_BITS
#error "FIXEDPT_WBITS must be less than or equal to FIXEDPT_BITS"
#endif

#define FIXEDPT_VCSID "$Id$"

#define FIXEDPT_FBITS (FIXEDPT_BITS - FIXEDPT_WBITS)
#define FIXEDPT_FMASK (((fpt)1 << FIXEDPT_FBITS) - 1)

#define fpt_rconst(R) ((fpt)((R) * FIXEDPT_ONE + ((R) >= 0 ? 0.5 : -0.5)))
#define fpt_fromint(I) ((fptd)(I) << FIXEDPT_FBITS)
#define fpt_toint(F) ((F) >> FIXEDPT_FBITS)
#define fpt_add(A, B) ((A) + (B))
#define fpt_sub(A, B) ((A) - (B))
#define fpt_xmul(A, B) ((fpt)(((fptd)(A) * (fptd)(B)) >> FIXEDPT_FBITS))
#define fpt_xdiv(A, B) ((fpt)(((fptd)(A) << FIXEDPT_FBITS) / (fptd)(B)))
#define fpt_fracpart(A) ((fpt)(A) & FIXEDPT_FMASK)

#define FIXEDPT_ONE ((fpt)((fpt)1 << FIXEDPT_FBITS))
#define FIXEDPT_ONE_HALF (FIXEDPT_ONE >> 1)
#define FIXEDPT_TWO (FIXEDPT_ONE + FIXEDPT_ONE)
#define FIXEDPT_PI fpt_rconst(3.14159265358979323846)
#define FIXEDPT_TWO_PI fpt_rconst(2 * 3.14159265358979323846)
#define FIXEDPT_HALF_PI fpt_rconst(3.14159265358979323846 / 2)
#define FIXEDPT_E fpt_rconst(2.7182818284590452354)

#define fpt_abs(A) ((A) < 0 ? -(A) : (A))

/* fpt is meant to be usable in environments without floating point support
 * (e.g. microcontrollers, kernels), so we can't use floating point types
 * directly. Putting them only in macros will effectively make them optional. */
#define fpt_tofloat(T)                                                         \
  ((float)((T) * ((float)(1) / (float)(1L << FIXEDPT_FBITS))))

#define fpt_todouble(T)                                                        \
  ((double)((T) * ((double)(1) / (double)(1L << FIXEDPT_FBITS))))

/* Multiplies two fpt numbers, returns the result. */
static inline fpt fpt_mul(fpt A, fpt B) {
  return (((fptd)A * (fptd)B) >> FIXEDPT_FBITS);
}

#if FIXEDPT_BITS == 64
static inline fpt div128_s64_s64(fpt dividend, fpt divisor) {
  fpt high = dividend >> FIXEDPT_FBITS;
  fpt low = dividend << FIXEDPT_FBITS;

  fpt result = div128_s64_s64_s64(high, low, divisor);
  return result;
}
#endif

/* Divides two fpt numbers, returns the result. */
static inline fpt fpt_div(fpt A, fpt B) {
#if FIXEDPT_BITS == 64
  return div128_s64_s64(A, B);
#endif
  return (((fptd)A << FIXEDPT_FBITS) / (fptd)B);
}

/*
 * Note: adding and subtracting fpt numbers can be done by using
 * the regular integer operators + and -.
 */

/**
 * Convert the given fpt number to a decimal string.
 * The max_dec argument specifies how many decimal digits to the right
 * of the decimal point to generate. If set to -1, the "default" number
 * of decimal digits will be used (2 for 32-bit fpt width, 10 for
 * 64-bit fpt width); If set to -2, "all" of the digits will
 * be returned, meaning there will be invalid, bogus digits outside the
 * specified precisions.
 */
static inline void fpt_str(fpt A, char *str, int max_dec) {
  int ndec = 0, slen = 0;
  char tmp[12] = {0};
  fptud fr, ip;
  const fptud one = (fptud)1 << FIXEDPT_BITS;
  const fptud mask = one - 1;

  if (max_dec == -1)
#if FIXEDPT_BITS == 32
#if FIXEDPT_WBITS > 16
    max_dec = 2;
#else
    max_dec = 4;
#endif
#elif FIXEDPT_BITS == 64
    max_dec = 10;
#else
#error Invalid width
#endif
  else if (max_dec == -2)
    max_dec = 15;

  if (A < 0) {
    str[slen++] = '-';
    A *= -1;
  }

  ip = fpt_toint(A);
  do {
    tmp[ndec++] = '0' + ip % 10;
    ip /= 10;
  } while (ip != 0);

  while (ndec > 0)
    str[slen++] = tmp[--ndec];
  str[slen++] = '.';

  fr = (fpt_fracpart(A) << FIXEDPT_WBITS) & mask;
  do {
    fr = (fr & mask) * 10;

    str[slen++] = '0' + (fr >> FIXEDPT_BITS) % 10;
    ndec++;
  } while (fr != 0 && ndec < max_dec);

  if (ndec > 1 && str[slen - 1] == '0')
    str[slen - 1] = '\0'; /* cut off trailing 0 */
  else
    str[slen] = '\0';
}

/* Converts the given fpt number into a string, using a static
 * (non-threadsafe) string buffer */
static inline char *fptoa(const fpt A) {
  static char str[25];
#if FIXEDPT_BITS == 64
  FP64_ToString(A, str);
#else
  fpt_str(A, str, 10);
#endif
  return (str);
}

static fpt atofp(char *num_string) {
  fptu n = 0;
  int sign = 0;

  for (int idx = 0; num_string[idx] != '\0'; idx++) {
    char c = num_string[idx];
    switch (c) {
    case ' ':
      continue;
    case '\n':
      continue;
    case '-':
      sign = 1;
      continue;
    default:
      if (is_digit(c)) {
        int digit = c - '0';
        n = n * 10 + digit;
      } else {
        dbg("Hit an unsupported character while parsing a number: '%c'", c);
      }
    }
  }

  return sign ? -n : n;
}

/* Returns the square root of the given number, or -1 in case of error */
static inline fpt fpt_sqrt(fpt A) {
  int invert = 0;
  int iter = FIXEDPT_FBITS;
  int i;
  fpt l;

  if (A < 0)
    return (-1);
  if (A == 0 || A == FIXEDPT_ONE)
    return (A);
  if (A < FIXEDPT_ONE && A > 6) {
    invert = 1;
    A = fpt_div(FIXEDPT_ONE, A);
  }
  if (A > FIXEDPT_ONE) {
    fpt s = A;

    iter = 0;
    while (s > 0) {
      s >>= 2;
      iter++;
    }
  }

  /* Newton's iterations */
  l = (A >> 1) + 1;
  for (i = 0; i < iter; i++)
    l = (l + fpt_div(A, l)) >> 1;
  if (invert)
    return (fpt_div(FIXEDPT_ONE, l));
  return (l);
}

/* Returns the sine of the given fpt number.
 * Note: the loss of precision is extraordinary! */
static inline fpt fpt_sin(fpt fp) {
  int sign = 1;
  fpt sqr, result;
  const fpt SK[2] = {fpt_rconst(7.61e-03), fpt_rconst(1.6605e-01)};

  fp %= 2 * FIXEDPT_PI;
  if (fp < 0)
    fp = FIXEDPT_PI * 2 + fp;
  if ((fp > FIXEDPT_HALF_PI) && (fp <= FIXEDPT_PI))
    fp = FIXEDPT_PI - fp;
  else if ((fp > FIXEDPT_PI) && (fp <= (FIXEDPT_PI + FIXEDPT_HALF_PI))) {
    fp = fp - FIXEDPT_PI;
    sign = -1;
  } else if (fp > (FIXEDPT_PI + FIXEDPT_HALF_PI)) {
    fp = (FIXEDPT_PI << 1) - fp;
    sign = -1;
  }
  sqr = fpt_mul(fp, fp);
  result = SK[0];
  result = fpt_mul(result, sqr);
  result -= SK[1];
  result = fpt_mul(result, sqr);
  result += FIXEDPT_ONE;
  result = fpt_mul(result, fp);
  return sign * result;
}

/* Returns the cosine of the given fpt number */
static inline fpt fpt_cos(fpt A) { return (fpt_sin(FIXEDPT_HALF_PI - A)); }

/* Returns the tangent of the given fpt number */
static inline fpt fpt_tan(fpt A) { return fpt_div(fpt_sin(A), fpt_cos(A)); }

/* Returns the value exp(x), i.e. e^x of the given fpt number. */
static inline fpt fpt_exp(fpt fp) {
  fpt xabs, k, z, R, xp;
  const fpt LN2 = fpt_rconst(0.69314718055994530942);
  const fpt LN2_INV = fpt_rconst(1.4426950408889634074);
  const fpt EXP_P[5] = {
      fpt_rconst(1.66666666666666019037e-01),
      fpt_rconst(-2.77777777770155933842e-03),
      fpt_rconst(6.61375632143793436117e-05),
      fpt_rconst(-1.65339022054652515390e-06),
      fpt_rconst(4.13813679705723846039e-08),
  };

  if (fp == 0)
    return (FIXEDPT_ONE);
  xabs = fpt_abs(fp);
  k = fpt_mul(xabs, LN2_INV);
  k += FIXEDPT_ONE_HALF;
  k &= ~FIXEDPT_FMASK;
  if (fp < 0)
    k = -k;
  fp -= fpt_mul(k, LN2);
  z = fpt_mul(fp, fp);
  /* Taylor */
  R = FIXEDPT_TWO +
      fpt_mul(
          z,
          EXP_P[0] +
              fpt_mul(
                  z, EXP_P[1] +
                         fpt_mul(z, EXP_P[2] +
                                        fpt_mul(z, EXP_P[3] +
                                                       fpt_mul(z, EXP_P[4])))));
  xp = FIXEDPT_ONE + fpt_div(fpt_mul(fp, FIXEDPT_TWO), R - fp);
  if (k < 0)
    k = FIXEDPT_ONE >> (-k >> FIXEDPT_FBITS);
  else
    k = FIXEDPT_ONE << (k >> FIXEDPT_FBITS);
  return (fpt_mul(k, xp));
}

/* Returns the hyperbolic tangent of the given fpt number */
static inline fpt fpt_tanh(fpt X) {
  fpt e_to_the_2_x = fpt_exp(fpt_mul(FIXEDPT_TWO, X));
  fpt sinh = e_to_the_2_x - FIXEDPT_ONE;
  fpt cosh = e_to_the_2_x + FIXEDPT_ONE;
  return fpt_div(sinh, cosh);
}

/* Returns the natural logarithm of the given fpt number. */
static inline fpt fpt_ln(fpt x) {
  fpt log2, xi;
  fpt f, s, z, w, R;
  const fpt LN2 = fpt_rconst(0.69314718055994530942);
  const fpt LG[7] = {fpt_rconst(6.666666666666735130e-01),
                     fpt_rconst(3.999999999940941908e-01),
                     fpt_rconst(2.857142874366239149e-01),
                     fpt_rconst(2.222219843214978396e-01),
                     fpt_rconst(1.818357216161805012e-01),
                     fpt_rconst(1.531383769920937332e-01),
                     fpt_rconst(1.479819860511658591e-01)};

  if (x < 0)
    return (0);
  if (x == 0)
    return 0xffffffff;

  log2 = 0;
  xi = x;
  while (xi > FIXEDPT_TWO) {
    xi >>= 1;
    log2++;
  }
  f = xi - FIXEDPT_ONE;
  s = fpt_div(f, FIXEDPT_TWO + f);
  z = fpt_mul(s, s);
  w = fpt_mul(z, z);
  R = fpt_mul(w, LG[1] + fpt_mul(w, LG[3] + fpt_mul(w, LG[5]))) +
      fpt_mul(z, LG[0] +
                     fpt_mul(w, LG[2] + fpt_mul(w, LG[4] + fpt_mul(w, LG[6]))));
  return (fpt_mul(LN2, (log2 << FIXEDPT_FBITS)) + f - fpt_mul(s, f - R));
}

/* Returns the logarithm of the given base of the given fpt number */
static inline fpt fpt_log(fpt x, fpt base) {
  return (fpt_div(fpt_ln(x), fpt_ln(base)));
}

/* Return the power value (n^exp) of the given fpt numbers */
static inline fpt fpt_pow(fpt n, fpt exp) {
  if (exp == 0)
    return (FIXEDPT_ONE);
  if (n < 0)
    return 0;
  return (fpt_exp(fpt_mul(fpt_ln(n), exp)));
}

#endif
