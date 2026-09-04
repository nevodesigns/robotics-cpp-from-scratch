# The Same Chart, Twice the Pixels: Logical and Device Coordinates

> At a ratio of 1 the correct version and the broken one are pixel for pixel
> identical. That is why it ships.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 10-01, 10-03

## The Problem

The plotter from 10-03 works. Somebody opens it on a better screen and the chart
is a quarter of the size, in the top left corner, with three quarters of the
window empty.

Nothing is wrong with the drawing code. There are two kinds of pixel, and only
one of them was ever mentioned.

## The Concept

### Two kinds of pixel

A widget 100 units wide, on a display that draws two device pixels for each of
them, is:

- **200 pixels** of memory, which is what the image is made of;
- **100 units** of arithmetic, which is what you paint in.

Every high resolution display bug is one of those being used where the other
belongs.

The image has to be sized in the first and told the ratio, so the painter
accepts the second:

```cpp
QImage canvas() const {
  QImage image(device(), QImage::Format_ARGB32);   // logical * ratio, rounded
  image.setDevicePixelRatio(ratio);                // the half people leave out
  return image;
}
```

Measured on a 100 by 60 chart:

| | image | ink | bounding box |
|---|---|---|---|
| ratio 1 | 100x60 | 396 | 0,0..99,59 |
| ratio 2, never told | 200x120 | **397** | **0,0..100,60** |
| ratio 2, told | 200x120 | 797 | 0,0..199,119 |

The middle row is the same image size as the last and a quarter of the drawing.

**Read the first and second rows together.** They are the same picture. At a
ratio of 1 the correct code and the broken code produce identical output, pixel
for pixel, so the fault is invisible to the person who writes it, invisible in
review, and invisible in a test that renders at 1.

Which means the test has to render at something else:

```cpp
const Surface plain{QSize(100, 60), 1.0};
const Surface better{QSize(100, 60), 2.0};
```

Four lines, and every version of this bug fails.

### Do not fix it by scaling the coordinates

The tempting repair, once the picture is small, is to multiply everything by the
ratio on the way into the painter. It puts the picture back and it breaks
everything else: hit tests, layout arithmetic, stored geometry, anything that
compares a coordinate against a size. Half the program is then in one unit and
half in the other, and which half is which is not written down anywhere.

Set the ratio. Leave the coordinates alone. That is what the ratio is for:
everything above that one line stays in the units the layout is in.

### Round the device size

A ratio is not always a whole number. 1.5 and 1.25 are ordinary settings, and
101 logical units at 1.5 is 151.5 device pixels, which does not exist.

```cpp
QSize device() const {
  return QSize(static_cast<int>(std::lround(logical.width() * ratio)),
               static_cast<int>(std::lround(logical.height() * ratio)));
}
```

So the mapping is not exactly invertible at fractional ratios, and a test that
demands an exact round trip through the size will fail at 1.5 for a reason that
has nothing to do with the code.

### A pen of width one is not one pixel

A horizontal line across a surface at a ratio of 2:

| pen | ink | device rows |
|---|---|---|
| `setWidth(0)` | 200 | **1** |
| `setWidth(1)` | 400 | **2** |
| `setWidth(2)` | 800 | 4 |

`setWidth(1)` means one **logical** unit, so the grid somebody tuned to be
unobtrusive at a ratio of 1 is twice as heavy at 2 and three times at 3, while
the text beside it stays the same weight because a font is laid out in logical
units and rendered at the device's resolution.

Width zero is the surprise. In Qt a pen of width zero is **cosmetic**: the
thinnest line the device can draw, not no line at all, and one device pixel at
any ratio.

So choose, per line, and say which:

- **A hairline** for rules, gridlines and borders whose job is to be visible and
  not noticed.
- **A real logical width** for a trace, or an outline that is part of the
  design, or anything that should look the same size on every screen.

`setWidth(1)` is what you get by not choosing: too thin to be a deliberate
thickness and too thick to be a hairline.

### Reading a coordinate back

A line drawn at logical `x = 75`, on a surface at a ratio of 2:

| | value |
|---|---|
| found in the image at column | **150** |
| read back with the ratio | 75 |
| read back without it | 150 |

The chart is 100 units wide, so 150 is not merely wrong, it is off the end of it.
Anything looking up "which sample is at 150" clamps to the last one or indexes
past the array.

```cpp
QPointF from_device(QPointF pixel) const {
  if (ratio == 0.0) return pixel;
  return QPointF(pixel.x() / ratio, pixel.y() / ratio);
}
```

Convert at the boundary, in one function, and the rule becomes reviewable: **a
coordinate that came out of an image is device, everything else is logical, and
there is one place where they meet.**

Qt already delivers mouse and touch positions in logical units, so a hit test
against an event needs no conversion. What needs it is anything that reads
pixels: a test asserting where a line landed, a screenshot compared against an
expectation, a colour picked out of a rendered frame.

Two habits: name the units in the variable, and assert that
`from_device(to_device(p))` is `p`.

## Build It

Implement `device`, `canvas` and `from_device` in `exercise/solution.hpp`.

```
rcpp verify 10-04
```

The suite renders the same chart at two ratios and compares what filled the
image, counts the device rows three pen widths ink, and finds a drawn line in an
image to read its position back.

Everything renders offscreen, so it runs on a machine with no display.

## Use It

**Keep one Surface per thing you paint into**, and take the ratio from the
widget rather than assuming it. A window dragged between two monitors changes
ratio while it is open.

**Render your tests at 2 as well as 1.** It is the same test twice and it is the
only thing that catches this class of bug before somebody with a better screen
does.

**Say which lines are hairlines.** Write it in the pen helper's name so it reads
at the call site.

**Never scale coordinates to compensate.** If a picture is the wrong size, the
ratio is not set somewhere; find that rather than multiplying.

## What Breaks First

- **A chart drawn at quarter size in the corner.** See `E-QT-0012`.
- **A one pixel line that is two pixels elsewhere.** See `E-QT-0013`.
- **A coordinate read back without the ratio.** See `E-QT-0014`.

## Ship It

`Surface` and `hairline` join `rc::qt` beside the path view and the live plotter.
Every drawing from here can be rendered at any ratio and tested at two of them,
which is the difference between a chart that is correct and one that is correct
on the machine it was written on.
