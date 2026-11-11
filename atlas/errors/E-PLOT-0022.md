id: E-PLOT-0022
title: White text on a colour that is nearly white
match: the average of the channels puts white text on green, and the contrast says black
match: expected this to be true: naive_worse
platforms: linux, windows
teaches: 10-13-the-line-nobody-could-see
---

## Symptom

A legend entry or a button is unreadable on one colour and fine on the others: the label is white on a bright green or yellow.

## Cause

The average of red, green and blue is not how bright a colour looks: green carries most of the light. Measured: pure green averages 85, so the average rule said white text, and its contrast ratio is **1.37** where black is **15.30**. Over 216 colours the average rule chose the worse of the two for **47**.

## Fix

Choose the text colour by contrast:

```cpp
return contrast_ratio(background, white) > contrast_ratio(background, black) ? white : black;
```
