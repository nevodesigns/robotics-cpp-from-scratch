# The Click Beside the Point: From a Pixel Back to a Sample, in Pixels and Not in Volts

> A click is a pixel. Map it back with the exact inverse of the map that drew the chart, and measure how near a sample is in pixels, never in volts and seconds together.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 10-06

## The Problem

A chart is useful when you can point at it: hover on a spike and read its time and value. Two things go wrong. The pixel has to be mapped back exactly, with y going down the screen while the value goes up. And the sample under the cursor has to be found by distance on the screen, which is not distance in the data.

## The Concept

### The map and its inverse

`to_pixel` puts `t0` at the left of the usable area (inside a margin), `t1` at the right, the lowest value at the bottom and the highest at the top. With a margin of 10, times 5 to 15 and values -2 to 6: the lowest value was at y = **90** and the highest at y = **10**, time 5 at x = **10** and time 15 at x = **990**. Out and back through `from_pixel`, **25** points were **0** off by more than 1e-9. See `E-PLOT-0016`.

### Pixels and not volts

A chart of 100 seconds across 1000 pixels and 1 volt up 100 pixels. Sample A at (50 s, 0.5 V) and B at (50.9 s, 0.9 V). A click at the time of A and the value of B:

| | raw units | pixels |
|---|---|---|
| A | 0.4 | 40 |
| B | 0.9 | 9 |

In raw units A is nearer, and in pixels B is, and B is the one under the cursor. A second and a volt cannot be added. See `E-PLOT-0017`.

### The tolerance

A click 5 pixels away with a tolerance of 8 picked the sample; exactly 8 picked it (inclusive), 8.1 did not, and a diagonal 8.49 did not. Nothing within the tolerance is -1, so that a click on empty chart selects nothing. On 1000 samples, one pixel apart, **143** clicks each on a sample picked that sample, **0** wrong, and a tie goes to the earlier index.

### Off the chart and degenerate

A sample outside `[t0, t1]` cannot be picked, even if a click is where it would have been, at x = -50. A usable width of zero maps back to `t0` without dividing, and a range with no height puts the point in the middle. See `E-PLOT-0018`.

## Build It

`to_pixel`, `from_pixel` and `pick` in `exercise/solution.hpp`.

```
rcpp verify 10-08
```

## Use It

`rc/plot/pick.hpp` joins its module.

```cpp
const rc::plot::ChartView view = current_view();
const int index = rc::plot::pick(samples, view, event_x, event_y, 8.0);
if (index >= 0) show_readout(samples[index].time, samples[index].value);
```

**What this does not do.** `pick` looks at every sample, so it is a million steps on a million samples, and a chart that picks on every mouse move wants to look only at the columns near the click (lesson 10-06 has them). It finds the nearest sample and not the nearest point on the line between samples. And it knows nothing of device pixels: lesson 10-04 is about the pixels that a click and a drawing disagree on.

## What Breaks First

- **A map that is not the inverse of the one that drew.** See `E-PLOT-0016`.
- **Nearest in the wrong units.** See `E-PLOT-0017`.
- **Samples off the chart that can be hit.** See `E-PLOT-0018`.

## Ship It

`rc/plot/pick.hpp` holds `ChartView`, `to_pixel`, `from_pixel` and `pick`.

The zoom of the next lesson changes the view, and this is how the cursor finds its data again afterwards.
