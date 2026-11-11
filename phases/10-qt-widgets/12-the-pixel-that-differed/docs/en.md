# The Pixel That Differed: Comparing Two Chart Images With a Tolerance and a Count

> Compare pictures with a tolerance for each pixel and a count of how many may differ, and say where the difference is.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-04, 05-11

## The Problem

Lesson 05-11 compared expected text exactly and updated it on purpose. A picture of a chart is harder: render it, compare it with a picture known to be right, and it differs the day a platform antialiases a line a tenth of a pixel differently. A comparison tolerant enough to pass that must still find the one pixel that is wrong.

## The Concept

### Exact is too strict

The same chart, a frame, a horizontal line and an antialiased diagonal, drawn a tenth of a pixel apart. On Qt 6.2: **150** pixels differ exactly, the largest difference in any channel is **16** of 255, and **0** differ by more than 64. A person sees the same picture. The numbers belong to the rasteriser of one Qt version, so the tests assert the shape, which is some difference, small, and none above 64. See `E-QT-0060`.

### And not blind

One red pixel at (37, 12) was found: **1** differing, bounds at 37, 12, **1** by **1**, a largest difference of **255**. At a tolerance of 254 it was still found, and at **255** it was not, since no channel can be more than 255 apart. A tolerance has to be a number below that, and a count of differing pixels is the second guard. Two pixels far apart gave bounds that hold both, so a failure can be looked at. See `E-QT-0061`.

### Sizes

100 by 50 against 100 by 51, both white, is a **size mismatch**, and matches at no tolerance and no count: a comparison that goes over only the pixels both images have would call them equal. See `E-QT-0062`.

### Formats

The same chart in ARGB32, RGB32, premultiplied and RGBA8888: **0** pixels differ, since each pixel is read through `QImage::pixel`.

### A count

Three stray pixels differed by **3**, passed a count of 5 and failed a count of 2. The chart moved by one pixel was **170** differing, which no count that allows stray pixels passes.

## Build It

`compare` and `images_match` in `exercise/solution.hpp`.

```
rcpp verify 10-12
```

## Use It

`rc/qt/image_diff.hpp` joins its module.

```cpp
const QImage got = render_chart();
const rc::qt::Diff diff = rc::qt::compare(got, expected_image, 32);
if (diff.size_mismatch || diff.differing > 20) { got.save("got.png"); /* and look at diff.bounds */ }
```

**What this does not do.** It compares pixels and not meaning, so a chart that is correct and drawn in a different font is different. It reads every pixel, which is fine for a test and not for a frame. It does not make the picture that is known to be right: lesson 05-11 says how an expected output is updated, and an image is no different, a person looks at it and says yes. And the tolerance and the count are chosen by whoever writes the test: too wide and it is blind, too narrow and it fails on every platform.

## What Breaks First

- **An exact comparison of an antialiased picture.** See `E-QT-0060`.
- **A tolerance that sees nothing.** See `E-QT-0061`.
- **Images of different sizes called equal.** See `E-QT-0062`.

## Ship It

`rc/qt/image_diff.hpp` holds `Diff`, `compare` and `images_match`.

The last lesson of this phase checks the window chart against the terminal chart, and this is how a picture is checked.
