# RCA: Kernel panic in `maccel_events` during fast mouse movements

**Issue:** [Gnarus-G/maccel#112](https://github.com/Gnarus-G/maccel/issues/112)

## Summary

The kernel panic is caused by an unhandled **divide-by-zero** in the fixed-point math path used to compute mouse speed. When two input frames are processed close enough together that the elapsed `ktime` interval rounds to zero milliseconds, `input_speed()` divides the movement distance by `0`, which executes a signed 128-bit `idivq` with a zero divisor inside an interrupt handler and crashes the kernel.

## Faulting instruction

The panic report points to `RIP = maccel_events+0x61e`. The disassembly of the faulting bytes (`48 f7 ff`) is:

```asm
idiv %rdi
```

This is the signed 64-bit division emitted by `fpt_div()` (the `div128_s64_s64` helper in `driver/fixedptc.h`). The divisor `%rdi` is loaded from the stack and corresponds to the `time_ms` argument passed to `input_speed()`.

## Code path

```
maccel_events()
  -> event()
       -> update_mouse_move() / get_x() / get_y()
       -> accelerate()
            -> ktime_get(); diff = now - last_time
            -> millisecond = fpt_div(diff, UNIT_PER_MS)
            -> f_accelerate(x, y, millisecond, ...)
                 -> input_speed(dx, dy, time_ms)
                      -> distance = magnitude({dx, dy})
                      -> speed = fpt_div(distance, time_ms)   <-- divide by zero
```

`accelerate()` is invoked on every `EV_SYN` report. `last_time` is a function-local `static ktime_t`, so it persists across calls and is updated to `now` immediately after computing the interval. If the next report arrives before `ktime_get()` has incremented (because reports are coalesced, batched, or the clock source briefly returns the same value), `now - last_time` becomes `0`.

`fpt_div()` does not check for a zero divisor; it directly performs the `idivq`. Because `maccel_events()` runs in IRQ context, the resulting divide-error exception cannot be handled and the kernel panics.

## Reproduction

A userspace reproduction of the same code path (with `DEBUG=0` to keep the kernel module small, but the same arithmetic) triggers a `SIGFPE` when `time_ms` is passed as `0`:

```c
int x = 127, y = 127;
f_accelerate(&x, &y, 0, args);   // time_ms == 0
// -> SIGFPE at the fpt_div in input_speed()
```

The same `idivq` faulting sequence appears in the built kernel module disassembly.

## Secondary divide-by-zero risks

The following also call `fpt_div()` on values that can be zero if parameter validation is bypassed (e.g. direct `/sys/module/maccel/parameters/*` writes), but they are not the trigger reported in #112:

- `args.input_dpi == 0` in `accel.h:110` (`dpi_factor = NORMALIZED_DPI / input_dpi`).
- `synchronous` mode with `gamma == 0` and `motivity == 1` in `synchronous.h:19` (`gamma_const = gamma / log(motivity)`). The Rust `maccel-core` parameter validator rejects `motivity <= 1`, but direct sysfs writes bypass it.

The core validation layer (`crates/core/src/params.rs`) does guard against `input_dpi <= 0`, `motivity <= 1`, `gamma <= 0`, `sync_speed <= 0`, `decay_rate <= 0`, and `limit < 1`, but the driver itself does not re-validate the raw `charp` module parameters.

## Why fast/wide movements in CS2 Surf trigger it

Surf gameplay produces rapid, large mouse sweeps. At high polling rates or when the input subsystem batches multiple HID reports, consecutive `SYN_REPORT` events can be handled in the same interrupt/softirq window with identical `ktime_get()` readings, causing the computed interval to round to zero exactly when the movement magnitude is large.

## Potential fixes (not implemented here)

1. **Guard `time_ms` in `accelerate()`**: skip acceleration or clamp `millisecond` to a minimum positive value before passing it down.
2. **Guard inside `input_speed()`**: if `time_ms <= 0`, return `0` or the previous speed instead of dividing.
3. **Make `fpt_div()` fault-tolerant**: detect a zero divisor and return `0`/max/saturated value. This is a broader safety net but may mask other bugs.
4. **Add driver-side parameter validation**: reject zero/invalid values for `INPUT_DPI`, `MOTIVITY`, etc. at parameter write time rather than relying on userspace tools.
