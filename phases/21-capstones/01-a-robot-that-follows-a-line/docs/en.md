# A Robot That Follows a Line: A Camera, a Blob, a PID and a Supervisor, and What a Delayed Camera and a Dead One Do to It

> A line follower is the oldest project in robotics. What is new is that the one you build here knows when it has lost the line, and stops.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated camera and robot)
**Prerequisites:** 20-12, 19-14, 15-06, 15-05, 14-01, 00-05, 01-02

## The Problem

Phase 21 is the part of the curriculum made to be shown. Each lesson is a small robot built from the pieces of the earlier phases, **run under the supervisor of phase 19**, with its behaviour measured when things go right and when they go wrong. The rule from the start of the curriculum is that every capstone uses the safety primitives, and each one's tests check that a fault becomes a stop.

The first is the one that every employer has seen, so the interest is in what it does that the usual one does not. A camera looks down at the floor; the line is found in the picture; a PID steers the robot to keep it in the middle. Then the camera is slow, the line has a gap, the camera dies, the speed goes up, and in each case the answer is a **number**, and where the robot cannot cope, a **stop**.

## The Concept

### The loop

At 50 frames a second:

1. `render_line` draws what a camera looking straight down would see: a picture of 64 by 48 pixels of 1 cm each, whose bottom row is 15 cm ahead of the robot and whose top is 63 cm ahead, with the line a bright stripe on a dark floor. (A real camera gives a picture, not a render; the rest of the loop is the same.)
2. `measure_line` (lesson 15-06) finds the bright blobs, takes the largest, and if it has at least 8 pixels reports how far **to the left** of the robot's line of travel the line is, at the middle of the picture.
3. A PID (14-01) turns the robot towards it: the setpoint is zero and the measurement is **minus** the offset, so a line to the left turns left.
4. `line_wheels` turns a speed and a turn rate into the two wheel speeds, and the robot of lesson 00-05 moves.
5. The supervisor of 19-10 is told what the camera is doing and where the robot is, and decides how fast it may go.

The camera measurement of a straight line with the robot at -0.10, 0.00, +0.05 and +0.10 m was **+0.10, 0.00, -0.05 and -0.10 m**, each within a centimetre, with 192 pixels in the blob. A dead camera, and a line 0.5 m away, both gave **not found**.

### The gain

8 m of the course, a sine of 0.3 m amplitude and 4 m wavelength, at 0.4 m/s, started 5 cm off the line, with `kd` of 0.2:

| kp | rms distance from the line | largest | reached |
|---|---|---|---|
| 1.5 | 0.0983 m | 0.1548 | 7.49 m |
| 3.0 | 0.0324 | 0.0668 | 7.60 |
| 5.0 | **0.0152** | 0.0452 | 7.65 |
| 8.0 | 0.0182 | 0.0452 | 7.68 |

No stop in any of them. The error falls by a factor of three as the gain doubles to 3, and again to 5, and then **rises**: kp of 8 is worse than 5. (The largest error of 0.0452 is the 5 cm the robot started with, and it is the same for all of the good ones.) This is lesson 14-01's result, now with a camera in the loop. See `E-CAP-0001`.

### Speed

At **0.8 m/s** the same controller, kp 5, had an rms error of **0.0454**: **3.0 times** that at 0.4 m/s. The line comes at the robot twice as fast, and the picture's lookahead of about 0.4 m is a smaller number of seconds. Speed is the first thing to turn down when a line follower wanders, and the supervisor's slow speed (half) is there for that. See `E-CAP-0001`.

### A delayed camera

A camera does not deliver a picture the moment it is taken: the sensor, the transfer and the processing add tens to hundreds of milliseconds. At **0.8 m/s** with kp 5:

| delay | rms error | |
|---|---|---|
| 0 ms | 0.0454 | |
| 120 ms | 0.0459 | |
| 200 ms | 0.0488 | |
| 300 ms | 0.0669 | largest 0.1639 |
| 400 ms | 0.1695 | **line lost, stopped at x 1.69 m** |

Up to 200 ms it hardly matters; at 300 ms it is 1.5 times worse, and at 400 ms the loop is **unstable**: the robot swings out until the line leaves the picture, three frames pass without it, the supervisor sees a faulted sensor, and the robot **stops**. The delay is exactly the matter of lesson 15-03 and 14-07. Nothing in the controller knows it is unstable; it is the supervisor that turns a runaway into a stop, which is the point of the supervisor. See `E-CAP-0002`.

At the start there is nothing yet in the pipe for the first frames of the delay. That is not a lost line: counting it as one would stop the robot at the start line with any delay of three frames or more, which is the first thing this code did.

### A gap in the line

A gap of **0.2 m** was driven across with a debounce of 3 frames: the picture covers 0.48 m of floor, so a gap shorter than that never empties it. A gap of **0.6 m** empties the picture, and:

| debounce | result |
|---|---|
| 3 frames (60 ms) | stopped at x 2.86 m |
| 10 (200 ms) | stopped at x 2.88 m |
| 25 (500 ms) | stopped at x 2.94 m |
| 50 (1000 ms) | **driven across**, reached 7.51 m |

A short debounce is a robot that is cautious about a dashed line; a long one drives on, straight, on its last command, for a second, which is 0.4 m at this speed and which is fine on a straight line and is not on a bend. **The debounce is a distance, not a number of frames**, and it should be chosen from the longest gap the course has and the tightest curve the robot may meet in it. See `E-CAP-0003`.

### A dead camera

The camera goes black at 10 s. The supervisor stops the robot at **10.04 s**, three frames later, with the cause `sensors`. How far it goes before it is still depends on the speed it had: **0.022 m** at 0.4 m/s and **0.122 m** at 0.8 m/s, **5.5 times** as far for twice the speed (more than the four times of a square law, which is the lesson of 19-04: the distance to stop grows at least with the square of the speed, and the half speed of a suspect sensor is reached later at the higher one). The black box (19-12) holds what led to it:

```
+0.015  +0.015  +0.015  none  none  none
```

three measured offsets, and then three frames where there was no line. An incident report that says "the camera failed" is worth less than this. See `E-CAP-0003`.

## Build It

`measure_line`, `line_sensor_state` and `line_wheels` in `exercise/solution.hpp`. The course, the renderer, the loop and the supervisor wiring are given: they are the integration.

```
rcpp verify 21-01
```

## Use It

`rc/capstone/line_follower.hpp` opens the module `rc::capstone`.

```cpp
LineConfig config;
config.kp = 5.0;
config.speed = 0.4;
config.lost_debounce = 3;
const LineResult r = follow_line(LineTrack(), config, LineFault(), 20.0);
if (r.stopped) report(r.causes, r.offsets_before_stop);
```

## Show It

What goes in the portfolio is not the video of it working. It is the **table**: the gain against the error, the delay at which it stops being stable, the stop distance at each speed, and the way you found each of them. A reader who sees that a robot **stops** at a 400 ms delay, and why, trusts the one that does not stop more than a reader who has only seen it work. Put one table, one failure and one number from the black box in the README, each with the command that produces it (`rcpp verify 21-01`).

**What this does not do.** It is a simulated camera over a flat painted line with no glare, no shadow, no lens, no motion blur and no junctions; a real line follower spends most of its effort on exactly those. The controller is one gain on the offset at one point of the picture: a better one looks at the line's **angle** as well, uses the picture at two distances, and slows for a curve it can see coming. The speed is constant except when the supervisor slows it. The fence is a simple band. And the camera is the only sensor: a robot with only one has the fault of lesson 19-07, a single point of failure whose loss is a stop, which is safe and is also the end of the run.

## What Breaks First

- **A gain chosen by eye at one speed.** See `E-CAP-0001`.
- **A delay that nobody measured.** See `E-CAP-0002`.
- **A debounce chosen in frames and not in metres.** See `E-CAP-0003`.

## Ship It

`rc/capstone/line_follower.hpp` holds `LineTrack`, `LineFault`, `render_line`, `LineMeasure`, `measure_line`, `line_sensor_state`, `LineWheels`, `line_wheels`, `LineConfig`, `LineResult` and `follow_line`.

This is the first of the ten capstones: the others are built the same way, from the earlier phases and the supervisor of phase 19, and ended by a whole robot.
