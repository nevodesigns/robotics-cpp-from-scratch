id: E-CTRL-0007
title: Overshoot measured from zero instead of from the target
match: overshoot is measured from the target, not from zero
match: a response that stays under the target has no overshoot
platforms: linux, windows
teaches: 14-04-choosing-the-gains
---

## Symptom

Overshoot figures that are far too large and never zero. A loop that lands
neatly on its target reports a hundred percent overshoot. A loop that stops
short reports sixty.

The numbers are unusable in the direction that matters: they cannot distinguish
a well tuned loop from a badly tuned one, because they are not measuring
overshoot.

## Cause

The peak was divided by the target rather than compared with it:

```cpp
overshoot = peak / target;      // the bug
```

That is the peak **as a fraction of** the target, which is 1.0 for a perfect
response and 1.8 for one that went eighty percent past.

Overshoot is the part **beyond** the target:

```text
target 1.0, peak 1.8   ->   0.8, or eighty percent
target 1.0, peak 1.0   ->   0.0
target 1.0, peak 0.6   ->   0.0, not minus forty percent
```

The last line is the second half of the same mistake. A response that never
reached the target did not overshoot by a negative amount; it did not overshoot.
Letting the number go negative means an average over several runs quietly
cancels a real overshoot against a shortfall.

## Fix

```cpp
double peak = output[0];
for (std::size_t i = 1; i < output.size(); ++i)
  if (output[i] > peak) peak = output[i];

overshoot = target > 0.0 ? (peak - target) / target : 0.0;
if (overshoot < 0.0) overshoot = 0.0;
```

Two details worth keeping.

**Seed the peak from the first sample, not from zero.** A target below zero, or
a response starting above it, both break a peak seeded at zero, and the failure
is silent.

**Guard a target of zero.** Overshoot as a fraction of nothing is not a
quantity, and dividing by it gives infinity rather than an error. When the
target really is zero, an absolute overshoot in the units of the signal is the
honest thing to report instead.
