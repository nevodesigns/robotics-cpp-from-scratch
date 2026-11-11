# The Line Nobody Could See: Contrast, Text on a Colour and a Palette That Survives a White Background

> Whether a colour can be read is a ratio, not a feeling. Compute it, pick text by it, and keep only the series colours that show.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 10-05

## The Problem

A chart with four signals needs four colours, and its labels are text on colours. Choosing them by eye on one monitor produces a legend where the yellow line cannot be seen on a white page, and a green button whose white label is unreadable.

## The Concept

### Luminance and contrast

The relative luminance of a colour runs from 0 for black to 1 for white. Each channel is made linear first, with a straight part near black and a power of 2.4 above it, and the three are weighted: red **0.2126**, green **0.7152**, blue **0.0722**. Mid grey (128) is **0.2159**: half the channel value is about a fifth of the light. See `E-PLOT-0024`.

The contrast ratio is (lighter + 0.05) over (darker + 0.05), the same either way round. Black on white is **21**, a colour on itself **1**. Text wants at least 4.5 and a line at least 3.

| background | white text | black text |
|---|---|---|
| pure green | **1.37** | **15.30** |
| pure blue | **8.59** | **2.44** |

### Text on a colour

"Dark means white text", judged by the average of red, green and blue, puts white on pure green: its average is 85. The contrast says black. Over a grid of **216** colours the average rule picked the worse of black and white for **47**, and the contrast rule's worst result was **4.67**, so the better of the two is always readable. See `E-PLOT-0022`.

### A palette

`series_color` is the Okabe and Ito set, eight colours chosen to stay apart for the common kinds of colour blindness, repeating after 8, with a negative index counting back (including the smallest int, with no overflow). Their contrast on white:

| index | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| ratio | 5.19 | 3.87 | 3.42 | 3.06 | 2.25 | 2.31 | **1.32** | 21.00 |

Only **5** of 8 reach 3: indices 0, 1, 2, 3 and 7. The yellow is a ratio of 1.32 against white: a line nobody can see. A palette that is good against itself is not good against the page. See `E-PLOT-0023`.

## Build It

`relative_luminance`, `contrast_ratio`, `text_on`, `series_color` and `usable_series` in `exercise/solution.hpp`.

```
rcpp verify 10-13
```

## Use It

`rc/plot/palette.hpp` joins its module.

```cpp
const std::vector<int> usable = rc::plot::usable_series(background, 3.0);
const rc::plot::Rgb line = rc::plot::series_color(usable[signal_number % usable.size()]);
const rc::plot::Rgb label = rc::plot::text_on(line);
```

**What this does not do.** It is the contrast of two colours and not how a colour looks: it does not simulate colour blindness, and two colours with a high contrast against white can still be hard to tell from each other. It says nothing of line width (a thin line wants more contrast than a thick one) or of the dark theme beyond giving the ratios for whatever background it is given. And it is sRGB in 8 bits, as the screen is.

## What Breaks First

- **White text on green.** See `E-PLOT-0022`.
- **A colour nobody can see.** See `E-PLOT-0023`.
- **Contrast from the raw channel values.** See `E-PLOT-0024`.

## Ship It

`rc/plot/palette.hpp` holds `Rgb`, `relative_luminance`, `contrast_ratio`, `text_on`, `series_color` and `usable_series`.

The window chart of the last lesson of this phase draws each signal in a colour from it.
