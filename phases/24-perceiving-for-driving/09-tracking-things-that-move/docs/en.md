# Tracking Things That Move: A Constant Velocity Filter for Each Object, Association by Distance, and the Three Ways It Goes Wrong

> A vehicle that sees a person only where the person is, and not where they are going, plans for a world that has already moved.

**Type:** Capstone
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-03, 24-04, 22-06, 21-10

## The Problem

Lesson 24-03 found objects in one scan. A scan is a moment, and the world is not still. A person who is crossing the path is at one place in this scan and another in the next, and a planner that plans from where they are plans for a world that has already moved. What it needs is where the person **will be**, and that needs a velocity, which no single scan gives. So the objects are followed from scan to scan, each with a filter that estimates its position and its velocity from a stream of noisy positions, and a record that says that the object here is the same object as the one there.

## The Concept

### A filter for a point that moves

`Kf4` is the Kalman filter of a point in the plane with the state `(x, y, vx, vy)` and a covariance that says how uncertain each is. `kf_predict` moves the state on by `dt` at constant velocity and makes it less certain by an **acceleration** of variance `accel_var` acting for that time: the velocity is allowed to wander, but not to jump. `kf_update` corrects it by a measured position of variance `meas_var`. A target moving at 1.2 and 0.5 m/s, measured to 5 cm every 0.1 s, had its velocity read by the filter to within **0.122 m/s** (root mean square); the difference of two measured positions, the obvious way to get a velocity, was **1.073 m/s** off, because a position error of 5 cm over 0.1 s is 0.7 m/s. The filter uses all of the past. See `E-VEH-0052`.

### Association

A frame has detections and the tracker has tracks, and each detection must be matched to the track it belongs to. `associate` takes the **nearest pair** of an unmatched track's predicted position and an unused detection within a gate, matches them, and repeats; a detection that matches no track starts a new one. The prediction is what makes it work: a target is looked for where it is going to be, not where it was.

### The life of a track

A new track is **tentative**: a detection might be noise. After `confirm_hits` detections (here 3) it is **confirmed**, and in the test it was confirmed at frame **3**, while a single false detection in the same scene was never believed: it was a tentative track for one frame and gone at the next. A confirmed track that is missed **coasts** on its prediction, for `max_misses` frames; a target hidden for **2** frames behind a post came out with the **same id**, and one hidden for **8** had been dropped and came out with a **new id**. Whether the new id matters depends on what is done with the old one. See `E-VEH-0053`.

### Where it goes wrong

Two targets that are close together can have their ids swapped by a nearest neighbour tracker: the detections of one are nearer the prediction of the other. A constant velocity filter predicts well through a plain crossing (two people who walk past each other at 5 cm kept their ids), and badly when the targets **change what they are doing while they are close**: two people who walk up to each other, stop to talk side by side 5 cm apart and walk on. The prediction at the moment they set off again is worth nothing, and the detections are 5 cm apart with 5 cm of noise. In 20 runs, with the people passing a metre apart there were **0** swaps, and standing 0.05 m apart, **5**. A method that does not make this mistake keeps more than one hypothesis about who is who. See `E-VEH-0054`.

### The use

A person walked across the path at 1.4 m/s, and the track's velocity predicted where they would be in 2 s to within **0.20 m**. That is what the planner needs: a position in the future to plan around.

## Build It

`associate` in `exercise/solution.hpp`.

```
rcpp verify 24-09
```

## Use It

`rc/perceive/track.hpp` opens nothing new in `rc::perceive`.

```cpp
rc::perceive::Tracker tracker(rc::perceive::TrackerConfig());
tracker.step(dt, centroids_of_the_clusters);
for (const rc::perceive::Track& t : tracker.tracks())
  if (t.confirmed) avoid(rc::perceive::predict_ahead(t, 2.0));
```

**What this does not do.** It is a constant velocity model: a person who turns or stops is predicted to go straight on, and the process noise is how much it is willing to be surprised. It tracks points, with no size, so two objects side by side are two points. The association is greedy, not optimal, and keeps one hypothesis. The gate is a distance and not one scaled by the uncertainty of the track. It knows nothing of what a thing is, and the clusters it follows can split and merge from one scan to the next, which a lesson on clustering has already said.

## What Breaks First

- **A velocity from two positions.** See `E-VEH-0052`.
- **A track that lives for ever, or dies at the first miss.** See `E-VEH-0053`.
- **Ids that swap when paths cross.** See `E-VEH-0054`.

## Ship It

`rc/perceive/track.hpp` holds `Kf4`, `kf_init`, `kf_predict`, `kf_update`, `Point2d`, `associate`, `TrackerConfig`, `Track`, `Tracker` and `predict_ahead`.

This ends phase 24 of the specialisation course, perceiving for driving: a scan to points, the ground, the obstacles, the costs, the passable ground, the holes, the distortion of a sweep, the camera's view of the floor, and things that move.
