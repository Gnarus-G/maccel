#ifdef DEBUG
#ifdef __KERNEL__
#include <linux/printk.h>
#else
#include <stdio.h>
#endif
#define DEBUG_TEST 1
#else
#ifndef __KERNEL__
#include <stdio.h>
#endif
#define DEBUG_TEST 0
#endif

#ifdef __KERNEL__
#define dbg(fmt, ...) dbg_k(fmt, __VA_ARGS__)
#else
#define dbg(fmt, ...) dbg_std(fmt, __VA_ARGS__)
#endif

#ifdef __KERNEL__
#define dbg_k(fmt, ...)                                                        \
  _Pragma(" clang diagnostic push")                                            \
      _Pragma("clang diagnostic ignored \"-Wstatic-local-in-inline\"") do {    \
    if (DEBUG_TEST)                                                            \
      printk(KERN_INFO "%s:%d:%s(): " #fmt "\n", __FILE__, __LINE__, __func__, \
             __VA_ARGS__);                                                     \
  }                                                                            \
  while (0)                                                                    \
  _Pragma(" clang diagnostic pop")
#endif

#ifndef __KERNEL__
#define dbg_std(fmt, ...)                                                      \
  do {                                                                         \
    if (DEBUG_TEST)                                                            \
      fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,   \
              __VA_ARGS__);                                                    \
  } while (0)
#endif
