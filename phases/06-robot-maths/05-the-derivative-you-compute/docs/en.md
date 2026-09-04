# The Derivative You Compute Is Not the Derivative

> Making the step smaller made the answer worse, until it had no correct digits
> left at all.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 06-01, 00-04

## The Problem

Robots differentiate constantly. Velocity from encoder counts, angular rate from
orientation, a Jacobian for an arm that has no closed-form inverse, a derivative
term in every controller.

The formula is the one from school, and it is three symbols long:

```
f'(x) ~ (f(x + h) - f(x)) / h
```

There are two entirely different problems hiding behind it, they want opposite
things from `h`, and the habits from one are wrong in the other.

## The Concept

### Two errors, pulling opposite ways

**Truncation.** A difference quotient is the slope of a chord, not a tangent.
The gap shrinks with the step, which is the error everyone expects.

**Cancellation.** `f(x + h)` and `f(x - h)` are nearly equal, and subtracting
two nearly equal doubles throws away the leading digits they had in common. This
error **grows** as the step shrinks, because there is less and less difference
left to keep.

Measured on `sin(x)e^(0.3x)` at `x = 1.2`, where the true derivative is
`0.920153739316478`:

| step | forward | central | complex step |
|---|---|---|---|
| 0.1 | 4.777e-02 | 2.574e-03 | 2.577e-03 |
| 1e-2 | 4.546e-03 | 2.576e-05 | 2.576e-05 |
| 1e-4 | 4.521e-05 | 2.576e-09 | 2.576e-09 |
| 1e-6 | 4.518e-07 | **2.055e-12** | 2.578e-13 |
| 1e-8 | **1.388e-08** | 8.329e-09 | 2.220e-16 |
| 1e-10 | 3.544e-06 | 2.137e-07 | 2.220e-16 |
| 1e-12 | 4.432e-04 | 1.101e-04 | 1.110e-16 |
| 1e-14 | 3.464e-02 | 1.331e-03 | 2.220e-16 |
| 1e-16 | **9.202e-01** | 9.202e-01 | 2.220e-16 |

The bottom left entry is not an approximation of anything. An error of 0.920 on
a derivative of 0.920 means every digit is wrong.

Each method has a best step, and it is exactly where the theory says:

| method | best step measured | theory | its error |
|---|---|---|---|
| forward | 1e-8 | sqrt(eps) = 1.5e-8 | 1.388e-08 |
| central | 1e-6 | cbrt(eps) = 6.1e-6 | 2.055e-12 |

**Use both sides.** One extra call, and six thousand times less error, because
the errors on either side cancel rather than add.

And divide by the span you actually got:

```cpp
const double above = x + h, below = x - h;
return (f(above) - f(below)) / (above - below);
```

`(x + h) - (x - h)` is not exactly `2h` in floating point, and using `2h` gives
away a digit for nothing.

### A step is only small relative to something

Doubles are spaced proportionally. Near 1.0 neighbours are 2.2e-16 apart; near
1e12 they are 1.2e-4 apart. So a constant step is a different step everywhere.

`x*x` with a fixed step of 1e-6:

| x | gap between doubles | x + h == x | relative error |
|---|---|---|---|
| 1e0 | 2.22e-16 | no | 1.000e-12 |
| 1e3 | 1.14e-13 | no | 1.071e-08 |
| 1e9 | 1.19e-07 | no | 4.000e-02 |
| 1e10 | 1.91e-06 | no | 6.384e-01 |
| 1e12 | 1.22e-04 | **yes** | **1.000e+00** |

At a trillion the step is smaller than the gap between the numbers, so `x + h`
is `x`, both sides are the same value, and the slope of `x*x` comes back as
zero. Silently.

```cpp
inline double suggested_step(double x) {
  const double scale = std::fabs(x) > 1.0 ? std::fabs(x) : 1.0;
  return std::cbrt(std::numeric_limits<double>::epsilon()) * scale;
}
```

The floor matters as much as the scaling: `eps^(1/3) * |x|` alone vanishes with
x, and at zero there is no step at all. With this, the same sweep is accurate to
about 1e-12 relative from 1 to 1e12.

Then assert it rather than trusting it:

```cpp
RC_CHECK(x + h != x);
```

### Not subtracting at all

There is a way out of the cancellation entirely, for a function you own.

```cpp
return f(std::complex<double>(x, h)).imag() / h;
```

`f(x + ih)` has the derivative sitting in its imaginary part, and getting it out
involves no subtraction of nearly equal numbers, so there is nothing to cancel.
The right-hand column above reaches one unit in the last place at a step of 1e-8
and stays there at 1e-100, where the forward difference has been returning
garbage for eight decades.

The restriction is real. The function must be analytic and must survive a
complex argument, which rules out `fabs`, `min`, `max`, any branch on the value,
and any black box. Where it applies it is free, and it is worth knowing that
"you cannot do better than sqrt(eps)" is a statement about a method rather than
about arithmetic.

### The other problem entirely

Now the same three symbols on measured data.

A position sensor accurate to a millimetre, read at 500 Hz, on a robot moving at
exactly 1 m/s:

| position noise | velocity rms | sigma*sqrt(2)/dt |
|---|---|---|
| 0.1 mm | 0.0705 m/s | 0.0707 |
| **1 mm** | **0.7049 m/s** | 0.7071 |
| 10 mm | 7.0487 m/s | 7.0711 |

Two samples are subtracted, so their noise adds in quadrature; then the whole
thing is divided by two milliseconds. **A millimetre becomes seven tenths of a
metre per second.**

Nothing is broken. That is what differencing does, and it is the number the
derivative term of the controller in lesson 15-01 was reacting to.

Notice which way the step goes here. For a function you can call, smaller is
better until floating point stops you. For measurements, **smaller is always
worse**, and there is no bottom to the V because the noise is not in the
arithmetic.

### Widen the interval, do not filter the result

The instinct is to smooth the noisy velocity. Measured, that is the worse of the
two options.

Differencing over N samples:

| N | lag | velocity rms | times better |
|---|---|---|---|
| 1 | 0.001 s | 0.7049 | 1.0 |
| 2 | 0.002 s | 0.3524 | 2.0 |
| 4 | 0.004 s | 0.1764 | 4.0 |
| 16 | 0.016 s | 0.0441 | 16.0 |
| 64 | 0.064 s | 0.0111 | 63.8 |
| 256 | 0.256 s | 0.0028 | 254.9 |

The noise falls as **N**, not as its square root, because the numerator's noise
does not grow while the denominator does.

Averaging a one-sample velocity over 256 samples would divide the noise by
sixteen, and it costs the same `N * dt / 2` of lag. Sixteen against two hundred
and fifty six, for the same price.

So: difference over as wide an interval as the lag budget allows, and filter
afterwards only if that is still not enough. Doing it the other way round, which
is the instinct, spends most of the budget for a sixteenth of the benefit.

And predict rather than tune:

```cpp
const double expected = rc::math::rate_noise(sensor_noise, span * dt);
```

Compare that against what the loop tolerates and against the lag budget from
lesson 15-03 **before** writing the loop.

## Build It

Implement `suggested_step`, `derivative` and `rate_noise` in
`exercise/solution.hpp`.

```
rcpp verify 06-05
```

The suite sweeps the step over sixteen decades for three methods, finds each
one's best step and checks it against the theory, differentiates at a trillion,
and then differences a noisy position six ways.

## Use It

**Central differences, with a scaled step**, everywhere you need the slope of a
function. A numerical Jacobian is this called once per column, and lesson 13-03
is where that gets used.

**Never a constant step.** Write `suggested_step(x)` or something like it, and
assert that the step survives being added.

**Treat measured rates as a separate problem.** Widen first, filter second, and
write down what noise you expect before you look at what you got.

**When the interval you need is wider than the lag you can afford**, stop
differencing. An estimator that fuses position with a motion model gives you a
velocity without amplifying anything, which is what phase 16 is for.

## What Breaks First

- **A step made smaller until the answer disappeared.** See `E-NUM-0008`.
- **A fixed step at a value it was never chosen for.** See `E-NUM-0009`.
- **A velocity differenced from a noisy position.** See `E-MATH-0006`.

## Ship It

`suggested_step`, `derivative`, `rate_over` and `rate_noise` join `rc::math`.
Every later phase that needs a slope has one that is right at any magnitude, and
every phase that needs a rate from a sensor has a formula for what it will cost
before the sensor is plugged in.
