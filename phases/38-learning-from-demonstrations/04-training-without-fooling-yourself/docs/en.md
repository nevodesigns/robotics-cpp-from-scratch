# Training Without Fooling Yourself: The Step Size, a Network That Learns the Noise, and Stopping on a Validation Set

> The training error always falls; the error that matters is on data the network has not seen, and it stops falling first.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-03, 38-02, 38-01

## The Problem

38-03 took one step against the gradient on one sample. Training is that step repeated over every sample, many times, and three things decide whether it works: how big each step is, how long it goes on, and what the result is judged on. The training error always falls if the step is small enough, and that is exactly why it is the wrong thing to watch. A network with more weights than the data can pin down will learn the noise of its training set, its training error going on down while its error on anything else goes up. This lesson trains the network of 38-03 and shows the three ways to fool yourself.

## The Concept

### Epochs and shuffling

An epoch visits every training sample once, taking one step on each, in an order shuffled afresh each epoch from a seeded stream (a Fisher-Yates shuffle). Without the shuffle, the network sees each episode's samples in a row and is pulled toward each episode in turn. With a seed, the same training gives the same weights every time, which is what makes a run checkable. See `E-LRN-0010`.

### The step size

With 16 hidden units for 20 epochs: a step of 0.001 left the test error at 0.00243, about twice the error of a good step after the same epochs; a step of 0.02 brought it to 0.00130; a step of 1.0 overshot every time, the weights grew without bound and the error became NaN. There is a band of steps that work, it is found by trying a few on the validation set, and the largest that is stable is usually near the edge of the band. A NaN in training is nearly always a step that was too large. See `E-LRN-0011`.

### Over-fitting

A network of 32 units trained on only 30 noisy samples for 3000 epochs: its training error fell from 0.04970 to 0.00012, and its error on a validation set of other episodes was lowest, 0.0280, at epoch 346, and then rose to 0.0448. After epoch 346 every step fitted the noise in those samples better and the task worse. More weights than the data can pin down, trained long enough, will always do this. See `E-LRN-0012`.

### Stopping on a validation set

The cure that costs nothing is to measure the validation error after every epoch, keep the weights of the best epoch, and stop when it has not improved for a while (the patience). With a patience of 200 epochs, training stopped at epoch 546 and gave back the weights of epoch 346. The validation set is a third part, separate from the test set: the test set is looked at once, at the end.

### Against the line

On demonstrations that reached the car's limit, split by episode, the network's test error was 0.00034 against 0.01240 for the straight line of 38-02. The network has bent where the line could not, which is the reason to have it.

## Build It

`train_epoch` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Shuffle `order` (for i from its size down to 2: j = the integer part of uniform(r) times i, swap order[i - 1] and order[j]); then for each index in that order, scale the sample's state with normalise and take one sgd_step toward its action.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- A step too small learns slowly, a good one learns, and one too large makes the weights blow up.
- On a small noisy set the training error keeps falling while the validation error turns and rises, and stopping on it keeps the best weights.
- Where the expert's turn is clipped, the network beats the straight line on episodes neither has seen.

```
rcpp verify 38-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/train.hpp`.

```cpp
rc::learn::Mlp net = rc::learn::make_mlp(16, 1.0, seed);
const rc::learn::TrainResult r = rc::learn::train_with_stopping(net, scale, train, validation, 500, 0.02, 50, seed);
```

What this does not do. The steps are plain gradient descent, one sample at a time, at a fixed step size; real training uses batches, a step size that changes, and a method (momentum, Adam) that adapts the step for each weight. Over-fitting has other cures besides stopping: more data (38-06 gets it where it is needed), fewer weights, and a penalty on the weights like the ridge of 38-02. The validation error is itself noisy, so the best epoch is a little lucky; patience smooths that, and a larger validation set more so.

## What Breaks First

- A model chosen by its training error. See `E-LRN-0012`.
- A step size that makes the weights blow up. See `E-LRN-0011`.
- Training that cannot be repeated. See `E-LRN-0010`.

## Ship It

`rc/learn/train.hpp` holds `sgd_step`, `train_epoch`, `mlp_mse`, `TrainResult` and `train_with_stopping`.
