# The Plot That Got No Room: Layouts, Fixed Heights and the Stretch That Gave It the Rest

> A layout shares a space equally unless told otherwise. Fix what must not change, let the plot expand, and give it a stretch.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-03, 09-01

## The Problem

A chart panel has a toolbar, a status bar and a plot. Put them in a layout and run the program: the plot is a third of the window, and so is each of the others, and making the window bigger makes all three bigger.

## The Concept

### The defaults

Three plain widgets in a vertical layout in a window 300 pixels high were **100**, **100** and **100**. The layout has no way to know that one of them is the one a person is looking at. See `E-QT-0054`.

### Who gets the room

- **A fixed height** keeps it: the toolbar is 30 and the status bar 20 at every window size.
- **Expanding** says that a widget wants any room that is going.
- **A stretch factor** divides what is left: the plot has 1 and the others have 0.

With these, the plot was **250** at 300 high and **450** at 500 high, and the toolbar and the status bar were **30** and **20** at both.

### Stacked and full width

The widgets are stacked in order with no gap (margins and spacing are 0): the toolbar at y = 0, the plot below it, the status bar below that and ending exactly at the bottom of the window. The plot was as wide as the window at 400 and at 640.

### The least it can be

A panel asked for the least it could be: **120 by 130**, which is the toolbar 30, the smallest plot 80 and the status bar 20, with the plot at least 120 wide. Asked to be 60 by 40 it was **120 by 130**: the layout sets the window's minimum. Without a minimum on the plot, the plot can be squeezed to nothing. See `E-QT-0055`.

### Hidden

Hiding the status bar gave the plot **270**; showing it took the plot back to **250**. A hidden widget takes no room, and the plot is what grows. See `E-QT-0056`.

## Build It

The constructor of `PlotPanel` in `exercise/solution.hpp`: the heights, the size policy, the minimum and the layout.

```
rcpp verify 10-10
```

## Use It

`rc/qt/plot_panel.hpp` joins its module.

```cpp
rc::qt::PlotPanel panel;
panel.show();
// draw into panel.plot(); put buttons in panel.toolbar() and a message in panel.status()
```

**What this does not do.** It is the arrangement and nothing in it: the three children are plain widgets. The heights 30 and 20 are in pixels, so on a screen with a larger font or a high DPI setting the toolbar may want more, and a toolbar that holds buttons should use their size hint rather than a number (lesson 10-04 is about logical pixels). The numbers are those of Qt on the offscreen platform; a platform with window decorations has the same layout inside a larger window.

## What Breaks First

- **Equal thirds.** See `E-QT-0054`.
- **A plot that can be made too small to see.** See `E-QT-0055`.
- **Room left behind by a hidden widget.** See `E-QT-0056`.

## Ship It

`rc/qt/plot_panel.hpp` holds `PlotPanel`.

The chart of lesson 10-03 is a widget, and this is the frame it sits in. The last lesson of this phase puts it there.
