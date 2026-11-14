# A Sampling Controller for Any Motion Model: Many Random Futures, Weighted by How Good They Are, and Why the Model Has to Be the Vehicle's

> Try many futures with the vehicle's own rules and steer by the ones that worked.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-04, 24-04

## The Problem

Pure pursuit and Stanley (26-04) are one formula each. A formula is fast and has no idea of obstacles, speed limits of the wheels, or a car that turns less than a differential drive. Each time the vehicle changes, a formula must be reworked. A sampling controller does the opposite: it knows nothing about the vehicle except a function that says where one step of control takes it, and it works out the rest by trying.

## The Concept

### Many futures, weighted

Keep a nominal sequence of controls (a speed and a yaw rate) over a horizon of 2 seconds. Every control step:

1. Make 100 copies with random noise added to each control.
2. Roll out each with the motion model and score it: squared distance from the path at every step; a big cost for a step in a blocked cell of the grid and a smaller one for costly cells; and a cost for the distance, at the end, from a point further along the path.
3. Weight each by `exp(-(cost - least) / lambda)`: the best are worth most, and lambda says how many of the others count.
4. Move the nominal by the weighted average of the noise, send the first control, shift the sequence by one.

That is model predictive path integral control in a plain form. Nothing in it needs a gradient, so the cost may be a lookup in a grid, a hard collision, anything.

### The model is a plug-in, and has to be the vehicle's

`step_model` has two models: a unicycle, which can turn on the spot, and a bicycle, whose yaw rate is limited to speed times its largest curvature (1.35 per metre for this car). With the car's own model, the car that began 1 m off the path ended 0.058 m from it, and its yaw rate never passed the car's limit. A controller that planned with a unicycle for the same car, on a path with a half circle of radius 1.5 m, had a mean error of 0.018 m against 0.047 m with the right model: hardly different. That was a surprise to the author, who expected the wrong model to track worse; it does not, because the controller plans ten times a second and every plan starts from where the car really is, so the wrong model is corrected before it can matter. Where it does show is in the prediction: the same 2 seconds of controls, a speed of 1 m/s and a yaw rate of 1.8 rad/s, took the unicycle 0.67 m from where the car ended (the car cannot turn that fast). A prediction that wrong cannot be relied on to check a stopping distance, an obstacle beyond the next second, or a speed plan. See `E-VEH-0094`.

### It knows the grid, so it goes round

A block of 0.6 m in the path: a controller given the grid made 0 poses in blocked cells and went round; one given nothing made 15. The cost of the cell is in the score, not in a separate planner, which is the main reason to use a sampler near things. See `E-VEH-0095`.

### More samples, less error, and the cost of it

With 8 samples the mean error over four seeds was 0.152 m; with 100 it was 0.077 m. Each control step costs samples times horizon rollouts, each against a window of the path: this is the price, and it is paid on the vehicle's processor every 0.1 s. See `E-VEH-0096`.

## Build It

The weighting and the update of the nominal in `mppi_control`, in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Weight_i = exp(-(cost_i - least cost) / lambda); then for each step k, add to the nominal v and w the weighted average over the samples of the noise that was added, clamped to the model's limits.

As shipped, 5 of the 6 tests fail, and these are the ones your code has to turn green:

- A car that starts to the side is brought onto the path with its own model, within its limits.
- The same controller drives a differential drive, which may turn harder.
- Planning with the wrong model hardly hurts tracking at ten plans a second, but its predictions are wrong.
- A controller that knows the grid goes round a block in the path, one that does not drives into it.
- More samples track the path better.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 26-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/drive/sampling.hpp`.

```cpp
rc::drive::MotionModel m;  m.bicycle = true;
rc::drive::MppiState st;
const rc::drive::Command c = rc::drive::mppi_control(m, cfg, pose, path, idx, &grid, st);
```

**What this does not do.** The noise is Gaussian and independent at every step, so the controls it tries are rough, which shows as a little jitter that a real actuator would filter. Lambda, the noise sizes and the weights are set by hand and matter a great deal (try halving lambda). It has no model of slip or of the steering lagging (the next lesson for a skid steer adds slip). It is not real-time guaranteed: the number of samples must be chosen against the processor, and the lesson times nothing. The result depends on the random numbers, so a test must use several seeds, as these do. The grid is read, never written, and the path is a list of points, not a time-parameterised trajectory.

## What Breaks First

- **The wrong model for the vehicle.** See `E-VEH-0094`.
- **A sampler with no grid.** See `E-VEH-0095`.
- **Too few samples.** See `E-VEH-0096`.

## Ship It

`rc/drive/sampling.hpp` holds `MotionModel`, `model_max_curvature`, `step_model`, `Lcg`, `MppiConfig`, `MppiState`, `Command`, `blocked_at`, `mppi_control`, `MppiRun` and `mppi_run`.
