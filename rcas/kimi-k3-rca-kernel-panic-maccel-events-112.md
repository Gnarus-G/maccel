# RCA: Kernel panic in `maccel_events` during fast mouse movements (CS2)

**Issue:** [Gnarus-G/maccel#112](https://github.com/Gnarus-G/maccel/issues/112)

## Summary

The panic is a `#DE` (divide error) raised inside interrupt context, which is fatal on x86. The faulting instruction at `RIP = maccel_events+0x61e` (bytes `48 f7 ff`) is:

```asm
idiv %rdi
```

executed with `RDI = 0` — i.e. a **signed 128÷64 division by zero**. That `idivq` is the hand-rolled helper `div128_s64_s64_s64()` in `driver/utils.h`, reached through `fpt_div()` → `div128_s64_s64()` (64-bit fixed point) in the `maccel_events` hot path.

The divide error has two sources, both unguarded:

1. **Divisor = 0** — the exact panic captured in the log.
2. **Quotient overflow** — when the ktime delta goes negative or pathological, the 128-bit dividend no longer fits the signed 64-bit quotient.

## Call path

```
maccel_events()                        driver/input_handler.h
  -> event(handle, v)
       -> accelerate(&x, &y)           driver/accel_k.h
            -> unit_time = ktime_to_ns(now - last_time)   // unclamped, can be <= 0
            -> millisecond = fpt_div(unit_time, UNIT_PER_MS)
            -> f_accelerate(x, y, millisecond, args)      driver/accel.h
                 -> dpi_factor = fpt_div(NORMALIZED_DPI, input_dpi)
                 -> speed_in = input_speed(dx, dy, time_ms)  driver/speed.h
                      -> distance = magnitude({dx, dy})
                      -> speed = fpt_div(distance, time_ms)  <-- #DE here
                 -> sensitivity(speed_in, args)
                      -> mode-specific fpt_div(...)          driver/accel/*.h
```

`maccel_events()` is registered as the `input_handler.events` callback and is executed from `input_pass_values()` → `input_event_dispose()` → USB/HID softirq, exactly as shown in the panic's `<IRQ>` call trace.

## Root cause

Every division in the interrupt path goes through the raw `idivq` helper with **no zero-divisor guard** and **no clamp on the ktime delta**.

### 1. `time_ms == 0` (most likely trigger)

`accelerate()` computes the inter-frame interval as `ktime_to_ns(now - last_time)` (`accel_k.h:63`) with `last_time` a `static ktime_t`. Two `EV_SYN` batches handled in the same tick (or batched by the input subsystem at high polling rates) yield `unit_time == 0`, so `millisecond = 0`. `input_speed()` then divides the movement `distance` by that `0` (`speed.h:25`), producing the `idiv %rdi` / `RDI=0` seen in the panic. Fast, bursty CS2 surf sweeps maximize the chance of batched reports landing on the same ktime reading while the movement magnitude is large.

### 2. Negative / overflowing ktime delta

`ktime_to_ns()` returns a **signed** `s64`. When events migrate across CPUs (multiple HID devices or IRQ re-affinity), `ktime_get()` is not strictly monotonic across cores, so `now - last_time` can be **negative**. `accel_k.h` passes this straight into `fpt_fromint()` and `fpt_div()` with no clamp, so a negative or pathological `time_ms` can also overflow the signed-64 quotient of the 128-bit idiv — a second, independent #DE source.

### 3. Zero divisor from module parameters

`atofp()` (`fixedptc.h:244`) silently returns `0` for any empty, negative, or non-numeric sysfs string (it only accumulates digits), and every driver parameter is a freely writable `charp` under `/sys/module/maccel/parameters/`. Unguarded divides that a `0` parameter can hit:

- `args.input_dpi == 0` → `fpt_div(NORMALIZED_DPI, input_dpi)` in `accel.h:110`.
- `natural` mode: `fpt_div(output, input_speed)` in `natural.h:46` when `input_speed == 0`.
- `linear` mode: `fpt_div(_x_square, x)` in `linear.h:19` when `x == 0`.
- `synchronous` mode: `gamma / ln(motivity)` when `motivity == 1` (ln = 0), and `1 / sharpness`, `1 / motivity` (`synchronous.h:19,25,26`).

The Rust `maccel-core` validator (`crates/core/src/params.rs`) rejects several of these invalid values at the CLI/TUI layer, but the driver does not re-validate raw sysfs writes.

## The green artifacts are incidental

The square green artifacts reported before the panic are GPU/display fallout from the hard IRQ hang, not the cause of the crash.

## Potential fixes (not implemented here)

1. **Guard `div128_s64_s64_s64` / `fpt_div`**: return `0` or a saturated sentinel on zero divisor (broad safety net, but may mask other bugs).
2. **Clamp the time delta in `accelerate()`**: treat `unit_time <= 0` as a minimum Δt or skip acceleration for that frame (`accel_k.h`).
3. **Bail out in `input_speed()`**: return `0`/previous speed when `time_ms <= 0` instead of dividing (`speed.h`).
4. **Sanitize parameters at the driver boundary**: reject/clamp `input_dpi <= 0`, `motivity == 1`, bad `smooth`/`sharpness`, etc., on parameter write rather than trusting `atofp()` output.
5. **Replace raw `idivq`** with the kernel's `div64_s64` / `div_s64_rem` helpers for defined, portable behavior.

Files to touch if a fix is later chosen: `driver/utils.h`, `driver/accel_k.h`, `driver/speed.h`, `driver/accel.h`, and the per-mode guards in `driver/accel/{natural,linear,synchronous}.h`.
