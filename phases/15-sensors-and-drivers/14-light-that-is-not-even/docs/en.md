# Light That Is Not Even: Local Thresholds and the Integral Image

> A threshold is a statement about the lighting. The lighting did not get the memo.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-06, 15-05

## The Problem

Lesson 15-06 found things in a frame by choosing a threshold, and Otsu's method chose it from the histogram, so it followed the lighting as it rose and fell. That works when the light is the same across the whole picture.

A lamp on one side of a bench is not. The left of the frame is dim and the right is bright, and a mark on the bright side is lighter than the plain background on the dim side. No single number separates marks from background. The question to ask of each pixel
is not whether it is dark but whether it is darker than the pixels round it.

## The Concept

### The local mean

For each pixel, take the mean of the box of pixels round it, and call the pixel a mark if it is clearly darker than that mean. Done by adding up the box for every pixel, it costs the box's area every time, so a box of 33 by 33 is over a thousand additions per pixel.

### The integral image

A **summed area table** removes that cost. Entry (x, y) holds the sum of every pixel above and to the left of it. The sum of any rectangle is then four entries:

```cpp
entry(x1 + 1, y1 + 1) + entry(x0, y0) - entry(x0, y1 + 1) - entry(x1 + 1, y0)
```

Build it once in one pass, and every box costs four reads however large it is. The table is one bigger each way, so a rectangle against the edge needs no special case.

2000 random boxes, some of them partly or wholly outside the picture, were checked against a pixel by pixel sum: none wrong. See `E-VIS-0010`.

### The edge

A box centred near the edge sticks out of the picture. Clip it and divide by the count of pixels that are actually in the picture, not by the whole box. On a flat picture of 90 the mean was 90.0 at the corner, where a box of 17 by 17 holds only 81 pixels of the picture, as everywhere else. Dividing by the whole box gives a mean that
falls off towards the corner, so a mark there is no longer darker than its surroundings and is missed. See `E-VIS-0010`.

### Where one threshold fails

The test scene is lit from the left, the background rising from 40 to 220 across the frame, with three dark marks 6 pixels thick, each 45 levels darker than the background around it.

| method | marks found | other pixels marked |
|---|---|---|
| one threshold, 121 from Otsu | 0.582 | 0.446 |
| local, radius 12 | 1.000 | 0.000 |

The one threshold found barely over half the marks, and called nearly half of the rest marks as well, the whole dim side. The local one found all and marked nothing else. See `E-VIS-0011`.

### The window

The window has to be bigger than a mark and smaller than the change in the light:

| radius | marks found | other pixels marked |
|---|---|---|
| 1 | 0.317 | 0.000 |
| 2 | 0.323 | 0.000 |
| 4 | 0.948 | 0.000 |
| 8 | 1.000 | 0.000 |
| 16 | 1.000 | 0.000 |
| 40 | 1.000 | 0.062 |
| 100 | 0.577 | 0.405 |

A window that fits inside a six pixel mark averages the mark with itself and sees nothing odd. One as wide as the picture is a global threshold again, and gets nearly the same 0.577 and 0.405 as before. Between them, 8 to 16 here, it works. See `E-VIS-0011`.

### The offset

On a real picture a pixel is a little darker than the mean half of the time, from noise alone. The **offset** asks for it to be darker by a margin. With noise of 6 grey levels:

| offset | marks found | other pixels marked |
|---|---|---|
| 0 | 1.000 | 0.269 |
| 4 | 1.000 | 0.136 |
| 8 | 1.000 | 0.052 |
| 10 | 1.000 | 0.030 |
| 14 | 1.000 | 0.008 |
| 20 | 0.987 | 0.001 |
| 30 | 0.755 | 0.000 |

With no offset more than a quarter of the background is marked. An offset of 20, a little under half the marks' contrast of 45, marked almost nothing wrongly and lost only 1.3 percent of the marks, and 30 began to lose the marks themselves. See `E-VIS-0012`.

## Build It

`IntegralImage` (its constructor, `box_sum`, `box_count` and `box_mean`) and `adaptive_threshold` in `exercise/solution.hpp`.

```
rcpp verify 15-14
```

The scene has padded rows, as a camera's does, and the noise comes from a seeded generator, so the figures above are the same on every run.

## Use It

`rc/vision/adaptive.hpp` joins `rc::vision`.

```cpp
const std::vector<std::uint8_t> marks = adaptive_threshold(frame, 16, 20);   // 255 on a mark
```

**What this does not do.** It finds marks darker than their surroundings; light marks on a dark ground need the comparison the other way round. It needs a radius chosen for the size of the thing, which the camera model of lesson 15-10 can supply from a distance, and an offset chosen for the noise and the contrast.
It marks pixels and does not group them: handing the mask to the blob finder of lesson 15-06 does that. A shadow that falls across a mark is just another change in the light and can fool it.

## What Breaks First

- **A sum that divides by the whole box at the edge.** Marks near the border are missed. See `E-VIS-0010`.
- **One threshold for an unevenly lit frame.** Half the picture is a mark. See `E-VIS-0011`.
- **No offset on a noisy picture.** A quarter of the background is marked. See `E-VIS-0012`.

## Ship It

`rc/vision/adaptive.hpp` holds `IntegralImage` and `adaptive_threshold`.

The mask is ready for the blob finder, which turns marked pixels into things at places.
