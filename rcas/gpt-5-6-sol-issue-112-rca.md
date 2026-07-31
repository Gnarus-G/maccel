# RCA: Kernel panic in `maccel_events` (#112)

**Issue:** [Gnarus-G/maccel#112](https://github.com/Gnarus-G/maccel/issues/112)

## Summary

The panic is conclusively caused by `input_speed()` dividing movement distance
by a zero time interval. The fixed-point division helper executes an unguarded
`idivq` in input/IRQ context, so the processor's divide-error exception becomes
a fatal kernel panic.

The panic log proves where the zero is consumed, but it does not contain enough
information to prove which event sequence produced it. The timing design has
multiple paths by which an invalid interval can arise: one global timestamp is
shared by all matching input devices and CPUs, equal timestamps are not
handled, and a nanosecond delta is narrowed to signed Q32.32 before being
converted to milliseconds.

## Evidence from the panic

The faulting bytes are:

```asm
48 f7 ff    idivq %rdi
```

At the fault:

```text
RDI = 0x0000000000000000
RDX:RAX = 0x0000000000000006:0x5fafadb500000000
```

`RDI` is therefore a zero divisor. The surrounding instructions reconstruct a
Q32.32 dividend, load the divisor from the stack, perform `idivq`, and then
store the quotient to a global variable. That sequence uniquely matches:

```c
fpt speed = fpt_div(distance, time_ms);
LAST_INPUT_MOUSE_SPEED = speed;
```

in `driver/speed.h:25-26`. This identifies `time_ms`, not a curve parameter, as
the zero divisor.

The call trace also places the failure in the expected path:

```text
USB/HID input
  -> input_event()
  -> input_event_dispose()
  -> input_pass_values()
  -> maccel_events()
```

Because this runs with the input device event lock held and interrupts
disabled, the divide error is fatal rather than a recoverable userspace
`SIGFPE`.

## Data flow

```text
maccel_events()                         driver/input_handler.h:70
  -> event(EV_SYN)                      driver/input_handler.h:23
       -> accelerate(&x, &y)            driver/accel_k.h:56
            -> now = ktime_get()
            -> unit_time = now - last_time
            -> millisecond = unit_time / UNIT_PER_MS
            -> f_accelerate(...)
                 -> input_speed(...)    driver/speed.h:14
                      -> distance / time_ms
```

No stage validates that the elapsed interval is positive before using it as a
divisor.

## Root cause

The immediate root cause is the missing invariant at the boundary of
`input_speed()`: `time_ms` is assumed to be nonzero, while its producer does
not guarantee that property.

The underlying timing design makes that invariant unsafe for three reasons.

### 1. Shared timing state

`accelerate()` stores its previous timestamp in a function-local static:

```c
static ktime_t last_time;
```

This is one module-wide value, not one value per `struct input_handle`. The
handler matches every device advertising `EV_REL`, and different devices have
different `event_lock` instances. Their callbacks can therefore access
`last_time` concurrently without serialization.

The same global-state problem also affects `MOVEMENT`, fixed-point carry, and
the last reported speed. It permits one device's event timing and movement
state to affect another device's calculation.

### 2. Equal intervals are unhandled

If consecutive calls observe the same `ktime_get()` value, then
`unit_time == 0`, the millisecond conversion remains zero, and
`input_speed()` executes `fpt_div(distance, 0)`.

An equal timestamp is possible in principle because clock resolution is not a
contract that every read must return a distinct value. Shared state and
concurrent callbacks widen the set of event orderings that must be handled.
The issue's panic confirms that a zero reached this point, but it does not
record timestamps or device identities, so this specific producer mechanism
cannot be declared proven from the report alone.

### 3. The delta is narrowed before unit conversion

For a 64-bit build, the code does this:

```c
s64 unit_time = ktime_to_ns(now - last_time);
fpt _unit_time = fpt_fromint(unit_time);
fpt millisecond = fpt_div(_unit_time, UNIT_PER_MS);
```

Q32.32 has only 31 positive whole-number bits plus a sign bit. Converting a
nanosecond count to Q32.32 before dividing by one million retains only the low
32 bits when assigned to `fpt`.

Consequences include:

- Intervals above roughly 2.147 seconds can become negative.
- Intervals alias modulo `2^32` nanoseconds, about 4.295 seconds.
- A delta whose low 32 bits are zero becomes exactly zero before
  `input_speed()` receives it.

This is independently incorrect even if it was not the exact trigger in the
reported run.

## Why the workload raises the probability

Fast movement increases the callback rate and the amount of nonzero movement
available when a bad interval occurs. Games may also change scheduling, USB
delivery timing, CPU affinity, and activity from other relative-input devices.
Those factors increase exposure to shared timing state, but wide movement does
not mathematically cause the zero divisor; it is a trigger condition for the
timing bug.

The green display artifacts are most plausibly fallout from the fatal kernel
exception and frozen graphics stack, not evidence that the GPU caused the
panic.

## Configuration assessment

The reporter used synchronous mode with valid values:

```text
SENS_MULT = 1
YX_RATIO = 1
INPUT_DPI = 1600
GAMMA = 1
SMOOTH = 1
MOTIVITY = 4
SYNC_SPEED = 15
ANGLE_ROTATION = 0
```

The machine code identifies the division before the synchronous sensitivity
calculation, so these values are not the source of this panic.

## Potential fixes

No fix was implemented as part of this RCA.

### Preferred root fix

Move timing and movement state into a per-handle context allocated by
`maccel_connect()`, and convert the integer nanosecond delta to milliseconds
without first representing raw nanoseconds as Q32.32. This removes cross-device
state coupling and the 4.295-second aliasing behavior.

### Required invalid-delta policy

Explicitly handle `delta <= 0` before acceleration. Possible policies are:

- Reuse the last valid interval.
- Accumulate movement until a positive interval is available.
- Apply only base sensitivity for that report.
- Clamp to a realistic minimum interval.

Accumulation preserves distance-over-time semantics best. Clamping to one
nanosecond avoids the panic but invents an extreme speed and can produce an
undesired maximum-gain event.

### Defensive boundary guard

Make `input_speed()` reject `time_ms <= 0`. This is appropriate defense in
depth because the function owns the division precondition, but it does not fix
the shared state or lossy conversion that generated the invalid value.

### Division-helper guard

A zero check in `fpt_div()` or `div128_s64_s64_s64()` would prevent this class
of processor exception globally. It should be a final safety net rather than
the only fix, because silently returning zero or saturation can conceal broken
call-site invariants and alter every fixed-point calculation.

### Driver-side parameter validation

The driver accepts writable string parameters and relies largely on Rust
userspace validation. Invalid direct sysfs writes can create other zero
divisors, including `INPUT_DPI == 0` and synchronous `MOTIVITY == 1`.
Validating parameters in the kernel module is separate hardening; it does not
explain the valid configuration in issue #112.

## Verification gaps

- The existing C tests pass but do not exercise a zero time interval.
- `driver/tests/div128_s64.test.c` explicitly leaves division by zero
  commented out because it crashes the test process.
- The natural zero-producing event sequence was not reproduced on the
  reporter's hardware.
- Confirming whether equal timestamps, cross-device concurrency, or Q32.32
  aliasing produced the reported zero requires temporary instrumentation of
  device identity, CPU, `now`, `last_time`, and the integer delta before any
  narrowing conversion.

## Conclusion

The panic itself is fully explained: an unchecked zero elapsed time reaches
the raw fixed-point `idivq` in `input_speed()` while running in IRQ context.
The durable correction is to make timing state per device, perform unit
conversion without Q32.32 overflow, define behavior for nonpositive deltas,
and retain a division guard as defense in depth.
