# Control Flow: Deciding and Repeating

> A motor command that jumps from zero to full power is how gearboxes die.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-01

## The Problem

An operator pushes a joystick from centre to full forward. Your code passes that
straight to the motors. The robot lurches, the wheels slip, the current spike
trips the battery protection, and if the robot is carrying anything it is now on
the floor.

The fix is not more power or a better motor. It is a small piece of code that
refuses to let a command change faster than the machine can survive. That code is
made of decisions and repetition, which is the whole of this lesson.

## The Concept

### Deciding

`if` runs a block when a condition is true, `else` runs another when it is not:

```cpp
if (speed > limit) {
  speed = limit;
}
```

Conditions compare with `==`, `!=`, `<`, `>`, `<=`, `>=`, and combine with `&&`
for and, `||` for or, `!` for not. The single most expensive typing mistake in
this language lives here: `=` assigns, `==` compares. Writing `if (x = 5)` sets
`x` to five and then treats five as true. It compiles.

### Clamping

Forcing a value into a range is so common it has a name:

```cpp
if (value < low)  value = low;
if (value > high) value = high;
```

The order matters when `low` is greater than `high`, which is a caller mistake
worth deciding about deliberately rather than by accident.

### Repeating

A `for` loop runs a block a known number of times:

```cpp
for (int i = 0; i < count; ++i) {
  // runs count times, with i counting from 0
}
```

Three parts separated by semicolons: where the counter starts, the condition that
must stay true, and what happens after each pass. A `while` loop is the same idea
with only the condition, used when you do not know the count in advance.

The loop counter starting at zero is not a style choice. Array positions in C++
start at zero, so a loop from `0` to `count - 1` visits exactly every element.
Starting at one and going to `count` reads past the end of the array, which is
the classic off by one bug, and it does not always crash. Sometimes it just reads
somebody else's memory and returns a plausible wrong number.

### Rate limiting

Now the real thing. A rate limiter takes the command you want, the command
currently applied, and the largest change allowed in one time step. It moves
towards the target by at most that much:

```
change = target - current
if change is larger than max_step, only move max_step
if change is more negative than -max_step, only move -max_step
otherwise take the whole change
```

The robot still reaches full speed. It takes a few hundred milliseconds to get
there, and the gearbox survives.

## Build It

Implement three functions in `exercise/solution.hpp`:

- `clamp(double value, double low, double high)` forces a value into a range.
- `rate_limit(double current, double target, double max_step)` moves current
  towards target by at most max_step.
- `steps_to_reach(double current, double target, double max_step)` counts how
  many rate limited steps it takes to arrive. Return 0 if already there, and
  return -1 if max_step is zero or negative, because that never arrives.

```
rcpp verify 01-02
```

## Use It

Every motor controller worth using has a rate limit, usually called a slew rate
or acceleration limit, sometimes implemented in the drive firmware and sometimes
in your code. Knowing it is three lines of arithmetic means you can tell the
difference between a drive that is protecting your hardware and one that is
hiding a problem.

Phase 14 puts this limiter between a PID controller and a motor, which is where
it belongs in a real system.

## What Breaks First

- **You wrote `=` where you meant `==`.** The assignment version compiles and is
  always true for any non zero value. See `E-CPP-0002`.
- **Your loop runs one time too many or too few.** Count from zero to less than
  the count. See `E-CPP-0007`.
- **Your rate limiter never arrives, or overshoots and oscillates.** You moved by
  max_step even when the remaining distance was smaller. See `E-NUM-0005`.
- **Your step count is one too many.** You compared two doubles with `!=`. Adding
  0.1 ten times does not land exactly on 1.0, so ask whether the remaining
  distance is small enough instead. See `E-NUM-0003`.

## Ship It

`clamp` and `rate_limit` are the first two entries in `rc::control`. The watchdog
in lesson 14-02 sits directly on top of them, and every actuator command in every
later phase passes through both.
