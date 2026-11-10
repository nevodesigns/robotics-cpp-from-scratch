# Where It Crosses Zero: Finding a Root by Halving, by Newton, and by Both When Newton Loses Its Way

> Many questions in robotics are "for what value is this zero": the angle that makes the torques balance, the time a
> trajectory reaches a distance. There is a method that cannot fail, a method that is fast, and a way to have both.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-08, 06-05

## The Problem

A joint angle that balances a load, the time at which a speed profile has covered a distance, the point at which a curve meets a wall: each is a value where some function is zero, and for most of them there is no formula.

Lesson 06-05 took the derivative of a signal, and the numbers it gave are the ingredients here. A root finder takes a function, and sometimes its slope, and says where it is zero. The easiest ones to write have a way of failing that is easy to miss.

## The Concept

### Halving

If a function has opposite signs at two ends, it crosses zero somewhere between them. Look at the middle: the half that still has opposite signs at its ends contains a crossing. Repeat. Each step halves the interval, so it needs about log2 of (width over tolerance) steps and cannot fail.

`bisect` on `cos(x) - x` over [0, 1] to 1e-12 took **40** steps and gave 0.739085133215. With both ends of `x - 3` above zero it reported **not converged**, an end that was exactly the root was returned at once, and the wide interval [-10, 100] still found 3. See `E-MATH-0017`.

### Newton

From a guess, go to where the tangent line crosses zero, and repeat. When the guess is close, the number of correct digits roughly doubles each step. On `cos(x) - x` from 1.0 the errors were 2.6e-01, 1.1e-02, 2.8e-05, 1.7e-10, and it was done in **4** steps.

Nothing keeps the step small. Where the slope is nearly flat the tangent crosses far away:

| function | from | what happened |
|---|---|---|
| cube root | 1 | the distance from zero doubled each step: 1.1e+12 after 40 steps, failed |
| arctangent | 2 | -2.3e+10 after 5 steps, failed |
| x^2 - 2 | 0 | the slope is zero, failed |

See `E-MATH-0016`.

### Both

Keep the root between `a` and `b`, where the function has opposite signs, as in halving. From the current point take a Newton step, and accept it only if it lands inside the bracket and the value of the function at least halved. Otherwise take the middle of the bracket. Either way the bracket shrinks.

Steps to a tolerance of 1e-12:

| function | halving | Newton alone | with a bracket |
|---|---|---|---|
| cos(x) - x | 40 | 4 | 4 |
| arctangent over [-3.3, 5.1] | 43 | fails | 5 |
| cube root over [-1, 2] | 32 | fails | 32 |

On the cube root every Newton step was refused, and the method was halving throughout, which is what it is for. See `E-MATH-0018`.

## Build It

`bisect`, `newton` and `safeguarded` in `exercise/solution.hpp`. The `Root` result and the function type are given.

```
rcpp verify 06-10
```

## Use It

`rc/math/roots.hpp` joins `rc::math`. The function type there is `ScalarFunction`.

```cpp
const Root r = safeguarded(f, df, 0.0, 1.0, 1e-12, 100);
if (!r.converged) { /* say why: no sign change, or too many steps */ }
```

**What this does not do.** It finds one root, and a function with several needs several brackets. It needs a bracket, which means knowing roughly where to look, and the slope has to be supplied (lesson 06-05 can compute one numerically when there is no formula, at some cost in accuracy). A root where the function touches zero without crossing, like
`x^2` at 0, has no sign change and is invisible to all of these. And a tolerance on the value of the function is not a tolerance on the answer: a very flat function can be near zero over a wide range.

## What Breaks First

- **Newton alone.** It runs away. See `E-MATH-0016`.
- **Halving with no sign check.** It returns a number that is not a root. See `E-MATH-0017`.
- **One method chosen for everything.** Slow or unsafe. See `E-MATH-0018`.

## Ship It

`rc/math/roots.hpp` holds `Root`, `bisect`, `newton` and `safeguarded`.

Lesson 06-07 solved systems that are linear. This is the same job for one unknown and a function that is not.
