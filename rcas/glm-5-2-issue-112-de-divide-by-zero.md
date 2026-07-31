# RCA: Kernel panic (`#DE` divide-by-zero) in `maccel_events` (#112)

**Issue:** [Gnarus-G/maccel#112](https://github.com/Gnarus-G/maccel/issues/112)

## Summary

A kernel panic occurs in `maccel_events` during fast, sweeping mouse
movements (e.g. CS2 surf maps). The fault is a `#DE` (divide error)
raised by a signed 64-bit `idivq` whose divisor is zero, executed in
interrupt context. The zero divisor is `time_ms` reaching
`input_speed()`, which divides movement distance by it without a guard.

## The fault

- **RIP:** `maccel_events+0x61e` (`[maccel]`)
- **Faulting bytes:** `48 f7 ff` → `idivq %rdi`
- **Registers at fault:**
  - `RAX = 0x5fafadb500000000` (≈ 6.37 in 32.32 fixed-point — the dividend low)
  - `RDX:RAX = 0x6:0x5fafadb500000000` (≈ 6.37 — movement distance)
  - `RDI = 0x0000000000000000` (the divisor — zero)
- **EFLAGS:** `0x00010016` (IF set, trap-from-interrupt context)
- **Context:** `<IRQ>` via `input_pass_values` → `input_event_dispose`
  → `input_event` → `hidinput_report_event` → `hid_irq_in` → USB IRQ

The dividend (~6.37) matches `distance`, the magnitude of a small
`dx,dy` movement in 32.32 fixed-point. The zero divisor is `time_ms`.

## Call path to the divide

```
maccel_events()                          input_handler.h:70
  -> event(EV_SYN)                        input_handler.h:32
       -> accelerate(&x, &y)              accel_k.h:56
            -> unit_time = ktime_to_ns(now - last_time)   accel_k.h:63
            -> millisecond = fpt_div(_unit_time, UNIT_PER_MS)  accel_k.h:74
            -> f_accelerate(x, y, millisecond, args)      accel.h:69
                 -> input_speed(dx, dy, time_ms)          speed.h:14
                      -> distance = magnitude({dx, dy})  math.h:11
                      -> speed = fpt_div(distance, time_ms)  speed.h:25  *** #DE ***
```

The only `idivq` on this path is `div128_s64_s64_s64` (`driver/utils.h:19`),
reached via `fpt_div` (`driver/fixedptc.h:160`). That helper emits a raw
`idivq %[B]` with no zero check, so a zero divisor raises `#DE`.

## Root cause

`accelerate()` uses a `static ktime_t last_time` to measure the interval
between `EV_SYN` reports:

```c
// accel_k.h:59-74
static ktime_t last_time;
ktime_t now = ktime_get();
s64 unit_time = ktime_to_ns(now - last_time);   // can be 0
last_time = now;
fpt _unit_time = fpt_fromint(unit_time);
fpt millisecond = fpt_div(_unit_time, UNIT_PER_MS);  // 0 / N == 0
...
return f_accelerate(x, y, millisecond, collect_args());
```

When `unit_time == 0`, `millisecond == 0`, and `input_speed()` then does
`fpt_div(distance, 0)` (`speed.h:25`) with no zero-guard. The signed
divide traps `#DE`; because `maccel_events` runs in IRQ context
(`<IRQ>` in the trace), the exception escalates to a kernel panic.

`unit_time == 0` happens when two consecutive `EV_SYN` events read the
*same* `ktime_get()` value. On TSC-based clocks, back-to-back reads in
the same interrupt can return identical timestamps, especially when HID
reports are coalesced/batched into one URB/IRQ.

This bug is **mode-agnostic**: the divide happens upstream of the curve
selection in `sensitivity()`, so it fires regardless of which
acceleration mode is configured.

## Why fast movements / CS2 surf trigger it

At 250–500 Hz polling with wide sweeps, the HID layer can batch multiple
`SYN_REPORT` events into a single URB, so `maccel_events` processes
several `EV_SYN`s in one call. On some platforms `ktime_get()` does not
advance between back-to-back reads in the same interrupt, so
consecutive SYNs read identical timestamps → `unit_time = 0` → `#DE`.

## Secondary divide-by-zero risks (same family)

Even if `time_ms` were guarded, other divisors on the same hot path can
still be zero when `input_speed == 0`:

- `__natural_sens_fun`: `fpt_div(output, input_speed)` (`natural.h:46`)
  — `#DE` if `input_speed == 0`.
- `linear_base_fn`: `fpt_div(_x_square, x)` (`linear.h:19`) — `#DE` if
  `input_speed == 0`.
- `__synchronous_sens_fun`: `fpt_ln(input_speed)` (`synchronous.h:32,49`)
  returns a sentinel on 0 (no crash), but downstream `fpt_pow`/`fpt_div`
  on those results can still hit zero divisors.

So the fault is one instance of a broader class: **unguarded fixed-point
divisions where the divisor (`time_ms`, and downstream `input_speed`)
can become zero on the hot path.** The reported crash is the
`input_speed(distance, time_ms=0)` case.

## Fix options (analysis only — not implemented)

1. **Floor the time interval** in `accelerate()`: treat `unit_time == 0`
   as a minimum epsilon (reuse the last non-zero `millisecond`, or clamp
   to 1 ns). Simplest, local, stops the panic at the source.
2. **Guard `input_speed()`**: if `time_ms <= 0`, return the last known
   speed (or 0) instead of dividing. Centralizes the guard but still lets
   `accelerate()` emit a zero interval to the curve functions.
3. **Guard every divisor at the curve layer** (`natural.h:46`,
   `linear.h:19`, `synchronous.h`): bail to `FIXEDPT_ONE` when
   `input_speed == 0`. Necessary to close the secondary cases above;
   redundant for the primary bug if (1) or (2) is done.
4. **Make `fpt_div`/`div128_s64_s64_s64` trap-free**: return a sentinel
   on zero divisor. Reject — masks real bugs and changes semantics
   kernel-wide for this module.

Recommended combination: **(1) + (3)** — floor the interval at the source
and add a zero-speed short-circuit in each curve's sensitivity function,
so the whole divide family is closed, not just the reported instance.