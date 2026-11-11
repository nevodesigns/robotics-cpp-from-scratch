# Ten Thousand Delegates: What a Repeater Builds, What a ListView Builds, and a Budget a Test Can Hold

> A Repeater makes an item for every row. A ListView makes them for the rows in view. Have the delegates report themselves and a test can hold the budget.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-05, 11-03

## The Problem

An interface lists the readings of a robot. With a hundred readings it is fine, and with a hundred thousand it takes seconds to appear and uses a gigabyte. Nothing is wrong with any line of the QML, and nothing reports it.

## The Concept

### A Repeater and a ListView

10000 rows, each shown by a delegate that tells a counter when it is made and when it goes:

| view | delegates made | alive |
|---|---|---|
| `Repeater` | **10000** | **10000** |
| `ListView`, height 300, rows of 30 | **21** | **21** |

The `ListView` made the ten rows that fit and a few more beyond them, and not the ten thousand. See `E-QT-0090`.

### Moving

Jumping to row 5000: **11** more delegates were made and **12** were alive afterwards, with **32** the most at once: the new ones are made before the old ones are given up. The view does not run out of delegates as it moves, and does not keep the ones it has left behind. See `E-QT-0091`.

### Independent of the rows

A `ListView` over **100**, **10000** and **1000000** rows had **21**, **21** and **21** delegates alive. A model of 10 rows had 10. The cost of the view is the height of the screen and not the length of the list.

### The counter

`DelegateCounter` keeps what was made (total), what exists (alive) and the most that existed (peak). Three made and one gone were 3, 2 and 3; made one more and gone three: 4, 0 and 3. A report of one going when none exist is **unmatched** and does not take alive below nothing: **2** unmatched. See `E-QT-0092`.

### A model that holds nothing

`RangeModel` computes each row when asked: row 1234 is 2468 and row 999999 is 1999998, for a model of a million rows that was never stored. A row past the end, an invalid index, a role that is not its own, and a negative count are each an empty answer or no rows.

## Build It

`DelegateCounter` and `RangeModel` in `exercise/solution.hpp`.

```
rcpp verify 11-11
```

## Use It

`rc/qt/delegate_counter.hpp` joins its module.

```cpp
engine.rootContext()->setContextProperty("delegates", &counter);
// in the delegate:  Component.onCompleted: delegates.created(); Component.onDestruction: delegates.gone()
RC_CHECK(counter.alive() < 60);       // a budget, held by a test
```

**What this does not do.** It counts delegates and does not measure their cost: a delegate with an image or a large tree of items is expensive for each one that exists, and the budget has to be chosen for the delegate. The numbers are for a view without a window, laid out by `forceLayout`; in a window the view also has a cache around what is visible, and the count is somewhat larger. The delegates have to report themselves, which is two lines in the QML. And a `Repeater` is right for a few items that all matter, such as the tabs of a bar.

## What Breaks First

- **A Repeater over a long list.** See `E-QT-0090`.
- **Delegates that never go.** See `E-QT-0091`.
- **A count that goes below nothing.** See `E-QT-0092`.

## Ship It

`rc/qt/delegate_counter.hpp` holds `DelegateCounter` and `RangeModel`.

The harness of the last lesson of this phase checks that an interface stays inside a budget.
