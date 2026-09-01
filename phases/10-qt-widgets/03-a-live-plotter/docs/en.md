# A Live Plotter: The Data Rate Is Not the Frame Rate

> The chart is correct, the arithmetic is the same arithmetic that worked in a
> terminal, and the application has stopped responding. Nothing is too slow.
> Something is being asked for too often.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 10-01
**Needs Qt:** yes, and lesson 10-02 is the version that does not

## The Problem

Lesson 10-02 built a strip chart and printed it. This puts it behind a widget
that updates while a robot is running, which is the point of having one.

Everything about the arithmetic carries over unchanged. What is new is a
question a terminal never asked: **how often should this be drawn?**

The obvious answer is: whenever there is new data.

```cpp
void addSample(double time, double value) {
  series_.add(time, value);
  update();                       // whenever there is new data
}
```

Point that at a sensor running at one kilohertz and the application stops
responding. Buttons take a second to react. The window does not move when
dragged. A profiler shows all the time inside painting, which looks like the
drawing being too slow, and it is not.

## The Concept

### Two rates, and only one of them is yours

A sensor produces data at whatever rate the hardware runs. A screen shows about
sixty frames a second. A person notices somewhere between ten and thirty.

Those are unrelated numbers, and the moment the drawing is tied to the data, the
fastest of them sets the pace for everything. At a kilohertz, nine hundred and
forty pictures a second are prepared and thrown away, and the event loop is busy
making them instead of handling the click that just arrived.

The fix is to stop tying them together:

```cpp
void addSample(double time, double value) {
  series_.add(time, value);
  stale_ = true;                  // and nothing else
}

void refresh() {                  // a timer at 30 or 60 Hz calls this
  if (!stale_) return;
  stale_ = false;
  update();
}
```

A thousand samples now cost one repaint.

### The other half: a frame with nothing new costs nothing

The `if (!stale_) return;` is not a micro optimisation, and leaving it out is
the second version of the same mistake.

A frame timer fires whether or not data arrived. A chart that repaints on every
tick regardless burns a core redrawing a picture identical to the last one, on a
robot where that core has other work. The stale flag is what turns the timer
from a schedule into a permission.

### update, not repaint

`update()` asks the event loop to paint when it next can, and merges several
requests into one.

`repaint()` paints **immediately and synchronously**, so nothing is merged and
the caller waits for it. It is the right call in approximately no situation that
arises in normal code, and it is what turns this problem from bad to
unrecoverable.

### A signal, and the first time Q_OBJECT does something

The widget announces its axis when the axis changes, so a label beside the chart
can follow it without polling:

```cpp
signals:
  void axisChanged(double low, double high);
```

Two things about that are worth noticing.

This is the first widget in the curriculum where `Q_OBJECT` is **load bearing**.
In lesson 00-08 the window had no signals, so removing the macro changed
nothing. Here the signal has no definition without it and the build fails at the
link, which is exactly the failure lesson 09-02 explained.

And it is announced **only when it changes**. A signal emitted on every paint
tells a listener the same thing sixty times a second, which is the repaint
problem again wearing a different hat.

### What did not change

The mapping, the rolling window, the axis rules, the flip. All of it came from
lesson 10-02 without a line altered, and the widget calls it.

By now that should be the expected result rather than a pleasant surprise, and
it is the reason the arithmetic and the surface were separated in the first
place: it means this lesson is about the one thing that is genuinely new.

## Build It

Implement `LivePlot` in `exercise/solution.hpp`:

- `addSample`, which records data and never repaints.
- `refresh`, which repaints only when something changed.
- `axis`, the range actually drawn, with room around it and a floor under it.
- announcing `axisChanged`, only when it changes.

```
rcpp verify 10-03
```

Most of the suite runs with no window and no screen, rendering into an image.
Three of the tests count repaint requests rather than looking at pixels, because
the thing this lesson is about is not visible in a picture.

## Use It

Put one beside anything that runs: the loop lateness from lesson 07-03, the
error a controller is working on from phase 14, the voltage on a link from
phase 08. A chart you can watch while something is happening answers a different
question from a number printed afterwards.

If the data arrives on another thread, one more rule applies: a widget may only
be touched from the thread that owns it. The crossing is what a queued signal is
for, and the queue from lesson 07-02 is the shape underneath it.

## What Breaks First

- **A repaint on every sample.** The event loop stops keeping up and the
  application appears to hang. See `E-QT-0009`.
- **A signal with no `Q_OBJECT`.** It links until somebody connects to it. See
  `E-QT-0001`.
- **An axis with no floor under its span.** A steady signal is drawn as violent
  noise. See `E-PLOT-0005`.

## Ship It

`LivePlot` joins `rc::qt`, and phase 10 has what it set out to build. Every
later phase now has a chart it can watch, and a learner without Qt has the same
chart in a terminal from lesson 10-02.
