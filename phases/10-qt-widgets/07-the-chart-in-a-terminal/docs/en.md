# The Chart in a Terminal: A Steady Signal Stays Steady and a Spike Stays a Spike

> A robot on a network has no screen and still needs a chart. The range is where a chart lies: give it a minimum span and a steady signal is a flat line.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 10-05, 10-06

## The Problem

Three earlier lessons made pieces: columns of lowest and highest values, a range with room around it, ticks on nice numbers. This puts them into the chart a robot with only a terminal can print, and finds the place a chart most easily lies: the range.

## The Concept

### A steady signal

A signal of 5 with noise of plus and minus 1e-6 in its last digit. With the range scaled to the data, the two values 2e-6 apart marked **11** of 13 rows, which is a storm. With a minimum span of 1, it marked **1** row, 40 stars wide, and the middle row was labelled 5.0:

```
    5.5 |                                        
        |                                        
    5.0 |****************************************
        |                                        
    4.5 |                                        
```

(Five of the thirteen rows shown.) See `E-PLOT-0013`.

### The range

`value_range` takes the lowest low and highest high of the columns that have samples, adds 5 percent of room each side, and makes it no narrower than `min_span`, centred. 1 to 3 became 0.9 to 3.1. A span of 10 over data from 1 to 3 was exactly 10 wide, centred on 2. With no samples it is 0 to 1.

`row_for` puts the top of the range on row 0 and the bottom on the last row, rounds, and keeps the answer inside the chart: 99 is row 0 and -99 the last row, and a range with no height puts everything in the middle. See `E-PLOT-0014`.

### A sine and a spike

Two periods of a sine in 40 columns and 13 rows marked **40** columns, rows **1 to 11**: every column has ink, with the padding above and below. 10000 quiet samples with a spike of 5 had the spike as the highest mark, alone, in row **1**: the 5 percent of room is the row above it.

### Labels

Each line was the same length. The ticks put **3** labels on 13 rows, and each was lower than the one above. A chart of nothing is still a chart: no marks. See `E-PLOT-0015`.

## Build It

`value_range`, `row_for` and `render_text` in `exercise/solution.hpp`.

```
rcpp verify 10-07
```

## Use It

`rc/plot/text_chart.hpp` joins its module.

```cpp
const std::vector<std::string> lines = rc::plot::render_text(samples, t0, t1, 60, 15, 0.1);
for (const std::string& line : lines) std::cout << line << "\n";
```

**What this does not do.** It draws one mark in each column and no line between columns, so a steep signal is a set of separate stars, and a column with no samples is a gap. The labels are on the rows of the ticks and nothing marks the time axis. `min_span` is a choice of the caller: 0.1 is right for a temperature and wrong for a voltage that moves by microvolts, and the chart does not know which.

## What Breaks First

- **A steady signal drawn as a storm.** See `E-PLOT-0013`.
- **A value outside the chart.** See `E-PLOT-0014`.
- **Labels off their rows.** See `E-PLOT-0015`.

## Ship It

`rc/plot/text_chart.hpp` holds `value_range`, `row_for` and `render_text`.

The same range, ticks and columns will draw the window version in the last lesson of this phase, so that the two charts cannot disagree.
