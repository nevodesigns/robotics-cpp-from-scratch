# A Million Points in 800 Columns: Drawing Every Sample Without Losing the Spike

> A chart has one column of pixels for each few hundred samples. Keep the lowest and the highest in each, and the spike is still there.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 10-05

## The Problem

A sensor logs at 1 kHz and an engineer opens a chart of the last 17 minutes: a million samples on a screen 800 pixels wide. Drawing them all is slow, and the common shortcut, drawing every 1250th, is fast and loses exactly what the engineer opened the chart to find.

## The Concept

### Every nth

A quiet signal of a million samples with one spike of 5 at sample 123457, which 1250 does not divide: every 1250th sample peaked at **0.001**. The spike is not in the picture. See `E-PLOT-0010`.

### A bin for each column

`decimate` cuts the time from `t0` to `t1` into one bin for each pixel column, and keeps for each the count of samples and the lowest and highest value. The same million samples gave **800** columns, **1000000** samples counted, **0** empty, and the highest of the columns' highs was **5**.

A sample on a boundary goes to the column on its right, and the sample at exactly `t1` goes to the last column and is not lost: that sample is why the count is a million and not 999999. Samples outside the range are not counted, and a range that is backwards counts nothing. See `E-PLOT-0011`.

### Against a brute force

5000 samples in no particular order, in 37 columns, were compared with a count made the slow way: **0** columns differed, and all **5000** samples were placed. The samples do not have to be sorted.

### The cost

A column is a vertical line from its low to its high, so it takes two points, or one if its values are equal. A million samples were drawn as **1600** points, which is two for each of the 800 columns, and the number does not depend on how many samples there were. See `E-PLOT-0012`.

## Build It

`decimate` and `points_to_draw` in `exercise/solution.hpp`.

```
rcpp verify 10-06
```

## Use It

`rc/plot/decimate.hpp` joins its module.

```cpp
const std::vector<rc::plot::Column> columns = rc::plot::decimate(samples, t0, t1, widget_width);
for (int x = 0; x < static_cast<int>(columns.size()); ++x)
  if (columns[x].n > 0) painter.drawLine(x, y_for(columns[x].low), x, y_for(columns[x].high));
```

**What this does not do.** It keeps the range of each column and not the path through it: two samples in one column that go up and then down look the same as down and then up. It does one pass over all the samples, so a million samples cost a million steps once per redraw, and a chart that redraws often wants to keep its columns and update only the new ones. And a column with no samples is empty and is drawn as a gap, not a line from its neighbours.

## What Breaks First

- **A spike lost to skipping.** See `E-PLOT-0010`.
- **A sample at the end that is not in a column.** See `E-PLOT-0011`.
- **A drawing that costs as many points as samples.** See `E-PLOT-0012`.

## Ship It

`rc/plot/decimate.hpp` holds `Column`, `decimate` and `points_to_draw`.

Lesson 10-02 kept a bounded window of samples. This draws that window, however long, in at most two points for each pixel.
