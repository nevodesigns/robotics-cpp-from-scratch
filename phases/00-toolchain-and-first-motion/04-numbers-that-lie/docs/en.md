# Numbers That Lie: Integer Division and Overflow

> The compiler will happily let you divide 3 by 4 and get zero, and your robot will drive into a wall because of it.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-01

## The Problem

An analogue to digital converter on a robot reports a raw count from 0 to 4095,
representing a voltage from 0 to 3.3 volts. Every robot in the world has one of
these. Converting that count to volts is the single most common line of code in
embedded robotics:

```cpp
double volts = (raw / 4095) * 3.3;
```

This line is wrong. It compiles without a single warning by default, it runs, and
it reports 0.0 volts for every reading except the maximum, where it reports 3.3.
An entire sensor reads zero, and nothing anywhere tells you why.

This lesson is about the small number of ways C++ numbers quietly lie, and the
habits that make them stop.

## The Concept

### Whole number division throws the remainder away

When both sides of a division are whole numbers, C++ performs whole number
division. The fractional part is not rounded. It is discarded.

```cpp
2000 / 4095        // 0, not 0.488
2000 / 4095 * 3.3  // 0.0, because the 0 happens first
```

The multiplication by 3.3 happens after the damage. The fix is to make at least
one side a fractional number before dividing, or to multiply before dividing:

```cpp
(raw * 3.3) / 4095      // multiply first, so the division is done on a double
raw / 4095.0            // one fractional operand makes the whole division fractional
static_cast<double>(raw) / 4095
```

The rule to carry: **divide last, or divide in doubles**.

### Whole numbers have edges, and going over wraps around

An `int` holds numbers up to about 2.1 billion. A 16 bit value holds up to
65535. Exceed the edge and the value does not saturate at the top, it wraps to
the bottom. An encoder count that wraps from 65535 to 0 makes a robot believe it
has instantly travelled backwards at enormous speed.

The trap is that the wrap often happens in an intermediate result you never see:

```cpp
int millivolts = raw * 3300 / 4095;   // raw * 3300 can be 13.5 million, fine for int
short small = raw * 3300 / 4095;      // but assigning into a 16 bit type is not
```

### Fractional numbers are not exact either

`0.1 + 0.2` is not `0.3`. Fractional types store values in binary, and 0.1 has no
exact binary form, exactly as one third has no exact decimal form. This is why
every test in this curriculum compares doubles with a tolerance:

```cpp
RC_CHECK_NEAR(volts, 1.65, 1e-6);   // correct
RC_CHECK_EQ(volts, 1.65);           // a trap
```

Never compare two computed fractional numbers for exact equality. Ask whether
they are close enough, and say how close.

## Build It

`exercise/solution.hpp` holds three functions, each with the bug described
above baked in. Your job is to fix all three so the tests pass.

- `adc_to_volts(int raw)` converts a count from 0 to 4095 into volts from 0.0 to
  3.3.
- `percent_of(int part, int whole)` returns the percentage, rounded to the
  nearest whole number, and returns 0 when `whole` is 0 rather than crashing.
- `average(const int* samples, int count)` returns the mean as a fractional
  number, and returns 0.0 for an empty array.

Run:

```
rcpp verify 00-03
```

## Use It

Production sensor code adds two more things on top of what you just wrote:
calibration, because no two sensors agree exactly, and filtering, because a
single reading is noisy. Both are later lessons. Neither can save a conversion
that was wrong before the filter ever saw it.

The habit that survives everything: when a number is suspicious, print the
intermediate values. The bug is almost never where the wrong answer appears. It
is one or two steps earlier, in a value you assumed was fine.

## What Breaks First

- **A conversion that always reports zero.** Whole number division ran before the
  multiplication. See `E-NUM-0001`.
- **A value that jumps to a huge or negative number for no reason.** An
  intermediate result overflowed its type. See `E-NUM-0002`.
- **A test that fails by 0.0000001.** You compared fractional numbers for exact
  equality instead of asking whether they are near enough. See `E-NUM-0003`.
- **The program stops dead with an arithmetic fault.** You divided a whole number
  by zero, which is not an error you can catch but a hardware fault. Guard the
  divisor. See `E-NUM-0007`.

## Ship It

These three functions go into `rc::core` as the first sensor conversion helpers
in your library. Every later phase that reads a real device depends on this
arithmetic being right, which is why it is taught before anything is read at all.
