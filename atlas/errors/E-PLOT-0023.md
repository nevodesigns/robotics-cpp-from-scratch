id: E-PLOT-0023
title: A series colour that cannot be seen on the background
match: on a white background only some of the palette can be seen, and the yellow is not one
match: expected this to be true: !has_yellow
platforms: linux, windows
teaches: 10-13-the-line-nobody-could-see
---

## Symptom

One of several lines is missing from a chart on a white background, and a user reports a signal that is not there. It is drawn, in yellow.

## Cause

A palette chosen to be different from itself is not chosen to show on the page. Measured: of eight colours on white, contrast **5.19, 3.87, 3.42, 3.06, 2.25, 2.31, 1.32** and **21**; the yellow at 1.32 is nearly invisible, and only **5** reach 3.

## Fix

Choose from the colours that have enough contrast with the background:

```cpp
if (contrast_ratio(series_color(i), background) >= minimum) out.push_back(i);
```
