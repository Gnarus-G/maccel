#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#ifdef __KERNEL__
#include <linux/printk.h>
#define dbg(fmt, ...) dbg_k(fmt, __VA_ARGS__)
#else
#include <stdio.h>
#define dbg(fmt, ...) dbg_std(fmt, __VA_ARGS__)
#endif

#if defined __KERNEL__ && defined __clang__
#define dbg_k(fmt, ...)                                                        \
  _Pragma("clang diagnostic push")                                             \
      _Pragma("clang diagnostic ignored \"-Wstatic-local-in-inline\"") do {    \
    if (DEBUG_TEST)                                                            \
      printk(KERN_INFO "%s:%d:%s(): " #fmt "\n", __FILE__, __LINE__, __func__, \
             __VA_ARGS__);                                                     \
  }                                                                            \
  while (0)                                                                    \
  _Pragma("clang diagnostic pop")
#elif defined __KERNEL__
#define dbg_k(fmt, ...)                                                        \
  do {                                                                         \
    if (DEBUG_TEST)                                                            \
      printk(KERN_INFO "%s:%d:%s(): " #fmt "\n", __FILE__, __LINE__, __func__, \
             __VA_ARGS__);                                                     \
  }                                                                            \
  while (0)
#else
#define dbg_std(fmt, ...)                                                      \
  do {                                                                         \
    if (DEBUG_TEST)                                                            \
      fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,   \
              __VA_ARGS__);                                                    \
  } while (0)
#endif
