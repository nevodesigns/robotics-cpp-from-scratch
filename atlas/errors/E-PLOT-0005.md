id: E-PLOT-0005
title: A steady signal drawn as violent noise
match: a steady signal autoscaled fills the whole chart
match: a minimum span widens an axis
platforms: linux, windows
teaches: 10-02-a-value-over-time
---

## Symptom

A reading that is known to be stable is drawn swinging from the top of the chart
to the bottom and back. Battery voltage, a temperature, a joint angle at rest:
anything held still looks alarming.

People respond by filtering the signal, which is the wrong fix for a problem
that is not in the signal.

The tell is the axis labels. Every one of them reads the same number:

```text
    12.0000 |############################################################
    12.0000 |
    12.0000 |
    12.0000 |
    12.0000 |############################################################
```

## Cause

The value axis was fitted to the data with nothing stopping it from collapsing.

Every real measurement moves in its last digit, because an analogue to digital
converter has a least significant bit and it is never perfectly still. An axis
fitted to the minimum and maximum of that expands until the wobble fills the
chart, and a change of one part in ten million is drawn exactly as large as a
change of one volt.

Measured in lesson 10-02: a battery reading steady to within **two microvolts**,
autoscaled, occupies more than fifteen of twenty rows.

Padding does not help, and it is worth understanding why. Padding adds a
fraction of the span, and a fraction of nearly nothing is nearly nothing:

```cpp
padded(Range{7.0, 7.0}, 0.5)   // still a span of zero
```

## Fix

Give the axis a minimum span, chosen from what the signal means:

```cpp
Range at_least(const Range& range, double minimum_span) {
  if (span(range) >= minimum_span) return range;
  const double middle = (range.low + range.high) / 2.0;
  return Range{middle - minimum_span / 2.0, middle + minimum_span / 2.0};
}
```

The number is a judgement about the signal rather than a constant to copy. For a
battery, a tenth of a volt is a change worth seeing and a microvolt is not. For
a joint angle it might be a degree. Writing it down is the point: it states what
counts as a change, which is a thing the chart cannot know and its author does.

Two checks worth having together, because one alone is easy to satisfy wrongly:

- A steady signal must **not** fill the chart.
- A real change must still be **visible** on the same axis rule.

A minimum so large that a genuine event disappears has traded one lie for
another.
