# A Small Network and Its Gradient: One Hidden Layer, Backpropagation Written Out, and a Check by Finite Differences

> A gradient that has not been checked against a difference quotient is a guess, and training on a wrong gradient fails slowly and quietly.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-02, 06-05

## The Problem

The line of 38-02 could not follow the expert where its turn was clipped. A learner that can bend is needed, and the usual one is a neural network: weighted sums passed through a function that bends, and weighted again. It is trained by moving every weight a little against the gradient of the error, so the gradient has to be right. A wrong gradient does not crash anything. Training goes on, the error falls a little or not at all, and nothing says why. This lesson writes the gradient out by hand for the smallest useful network and checks it against a difference quotient, the check every hand-written gradient needs.

## The Concept

### One hidden layer

Two inputs (the offset and the heading, scaled as 38-01 showed), a layer of hidden units and one output. Each hidden unit takes a weighted sum of the inputs plus a bias and passes it through tanh, which is close to a straight line near zero and flattens toward plus or minus one. The output is a weighted sum of the hidden units plus a bias. With enough units the sum of flattened curves can follow a clipped line, and much more. A network of 8 units has 33 weights: two inputs and a bias for each unit, an output weight for each, and one output bias.

The weights start random, from a seed, so the same seed always gives the same network (00-12). They must not start equal: units with the same weights get the same gradient and stay the same for ever.

### The gradient, backwards

The error for one sample is half the squared difference between the output and the demonstrated action. Its derivative with respect to each weight comes from the chain rule, worked from the output back:

- the output's error is g = y - target, and that is the gradient of the output bias;
- an output weight's gradient is g times its unit's activation;
- a unit's pre-activation receives g times its output weight times the derivative of tanh, which is 1 - tanh squared;
- each input weight's gradient is that times its input, and the unit's bias gets it unchanged.

That is backpropagation: one pass forward to get every activation, one pass back to share the error out. It costs about as much as the forward pass, for every weight at once. See `E-LRN-0007`.

### Checking it

For every weight in turn: nudge it up by a small h, compute the error, nudge it down by h, compute again, and divide the difference by 2h. That central difference is the derivative to about h squared. Over all 33 weights the backpropagated gradient agreed with the difference to better than one part in a million (the test requires a relative difference below 1e-6). With one mistake put in on purpose (the 1 - tanh squared left out, the most common slip) the difference was 0.123, and the check would have caught it. See `E-LRN-0008`.

### A step

Moving every weight by a step of 0.1 against its gradient took the error on that sample from 0.03014 to 0.01838. Training (38-04) is that step, repeated over the samples, many times.

## Build It

`mlp_gradient` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- With e = y - target: the output bias gets e; each output weight gets e times its unit's activation z[i]; each unit's pre-activation gets d = e w2[i] (1 - z[i]^2), and then its two input weights get d x[0] and d x[1] and its bias gets d.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- The backpropagated gradient agrees with finite differences for every weight.
- A gradient with one mistake in it is caught by the check, and a small step against the right gradient lowers the error.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 38-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/mlp.hpp`.

```cpp
rc::learn::Mlp net = rc::learn::make_mlp(16, 1.0, seed);
const double worst = rc::learn::numeric_check(net, x, target, 1e-5);   // run it whenever the gradient code changes
```

What this does not do. It is one hidden layer, one output and one sample at a time; real networks have several layers, many outputs and batches, and are differentiated by a library that builds the backward pass for you. The check is the same in every case. tanh is one choice of the bending function; others (the rectified linear unit) train faster in deep networks and have a kink the finite difference has to be kept away from. The finite difference check costs two forward passes per weight, which is why it is a test and not part of training.

## What Breaks First

- A gradient that was never checked. See `E-LRN-0008`.
- The derivative of the activation left out. See `E-LRN-0007`.
- Weights that all start the same. See `E-LRN-0009`.

## Ship It

`rc/learn/mlp.hpp` holds `Mlp`, `make_mlp`, `param_count`, `mlp_forward`, `MlpGrad`, `mlp_gradient` and `numeric_check`.
