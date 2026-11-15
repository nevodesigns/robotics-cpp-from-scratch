# A Policy That Is a Straight Line: Least Squares on the Demonstrations, the Gains It Finds, and the Limit It Cannot Fit

> The simplest learner that works is the one to beat, and where it fails tells you what the data holds that a line does not.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-01, 06-07, 06-08

## The Problem

Before a network, a line. A learned policy that is a weighted sum of its inputs is the simplest that can work, it is fitted in one step with no tuning, and it says something a network will not: what part of the expert's behaviour is a straight line, and what part is not. If a network later does no better than the line, the network has not learned anything the line did not. And when the line fails, where it fails tells you what the data holds that a line cannot.

## The Concept

### Least squares, three numbers

The policy is a turn rate w0 times the offset plus w1 times the heading error plus a constant b. The three are chosen to make the sum of the squared differences from the demonstrated actions as small as possible. Setting the derivative of that sum to zero gives three linear equations, the normal equations: the matrix is the sum, over the samples, of the feature vector (offset, heading, 1) times itself, and the right-hand side the sum of the feature vector times the action. They are solved by the elimination of 06-07, with pivoting so that a small pivot does not wreck the result. A set in which the features never vary enough (one state seen many times) gives a singular matrix, and the fit gives the zero policy rather than a number that means nothing.

### Where the expert is a line, the fit finds it

From demonstrations that started near the centre and never reached the car's largest turn (0.0 percent of the actions at the limit), the fit found -2.000 and -3.000: the expert's own gains, to the third decimal, with an error of 0.000000. In that range the expert is a straight line, and the line recovers it. See `E-LRN-0004`.

### Where the expert is clipped, the line cannot follow

From demonstrations that started further out, 8.4 percent of the actions were at the limit, where the expert's turn stops growing. A line cannot stop growing. The best line is a compromise: weights of -1.044 and -1.576, smaller than the expert's, too strong at the limit and too weak near the centre, with an error of 0.01816 that no line removes. Moving either weight by five percent made the error larger, which is what least squares promises: the best line, not a good policy. See `E-LRN-0005`.

### A ridge

Adding ridge times the sum of the squared weights to the error pulls the weights toward zero. It steadies a fit on few or nearly collinear samples, at a cost: a ridge of 100 took the weights to -0.592 and -0.614 and the error to 0.04677. The constant is left alone, since pulling it toward zero would only add a bias. The ridge is a choice, made on the validation set, not a fixed part of the method. See `E-LRN-0006`.

### Tested on new episodes

On demonstrators with their own styles, split by episode as 38-01 requires, the line's error was 0.00285 on the training episodes and 0.00316 on the test ones. This is the number a network has to beat.

## Build It

`fit_linear` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- With the features (offset, heading, 1), add up the normal equations A = sum f f^T and r = sum f a over d; add ridge to the first two diagonal terms (not the constant's); solve with solve3 and return the three numbers as w[0], w[1] and b (an empty or singular set gives the zero policy).

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- From demonstrations that never reach the limit, least squares finds the expert's own gains.
- Where the expert's turn was clipped, the line's gains shrink and an error is left that no line removes, and a ridge shrinks them further.
- On demonstrators with their own styles the line is tested on episodes it has not seen, and an empty set gives the zero policy.

```
rcpp verify 38-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/linear_policy.hpp`.

```cpp
const rc::learn::LinearPolicy p = rc::learn::fit_linear(train, 0.0);
const double baseline = rc::learn::linear_mse(p, test);   // what a network must beat
```

What this does not do. The features are the raw state; a line in other features (the state's squares, a clipped state) can fit more, and choosing them by hand is what a network does for itself. The fit is batch, all the data at once; one that updates with every sample exists (recursive least squares) and is not needed here. Least squares is pulled by outliers, as 06-08 showed; demonstrations with a few wild actions need a robust fit or a cleaned set.

## What Breaks First

- A baseline that was never fitted. See `E-LRN-0004`.
- A linear policy for an expert that saturates. See `E-LRN-0005`.
- A ridge chosen on the test set. See `E-LRN-0006`.

## Ship It

`rc/learn/linear_policy.hpp` holds `LinearPolicy`, `linear_act`, `solve3`, `fit_linear`, `linear_mse` and `saturated_share`.
