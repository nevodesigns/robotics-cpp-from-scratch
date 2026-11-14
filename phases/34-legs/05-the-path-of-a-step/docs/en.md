# The Path of a Step: A Swing That Starts and Ends Slowly, Clears the Ground, and Asks the Servos for Less Than They Have

> A foot should leave the ground gently, go over what is in the way, and land gently, and the servos set how quickly it may do all three.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-04, 34-01, 30-06, 29-07

## The Problem

34-04 said when a foot goes up and where it should land. Between the two the foot has to get there, over whatever is in the way, without a jolt at the start or the end (a jolt at touchdown is a hit on the ground, and the robot's own body feels it), and within what the servos can do (29-07, 29-08). The path of the step is the answer: a curve of the foot's place in time.

## The Concept

### The swing

From where the foot leaves (p0) to where it lands (p1), over the swing time, the foot moves along the ground by the minimum jerk curve of 30-06: 10 tau cubed less 15 tau to the fourth plus 6 tau to the fifth, which has zero speed and zero acceleration at both ends; and in height by 64 h tau cubed (1 - tau) cubed, which is zero with its first two derivatives at both ends, and h at the middle. The foot left at 0.000 m/s and landed at 0.000 m/s, a step of 0.10 m, 0.05 m high in 0.25 s. A path that goes straight up and down at the same height lands at 0.80 m/s: a hit on the ground. See `E-LEG-0012`.

### The stance

In the body's frame a foot on the ground goes straight back at the body's speed (34-04), so the stance path is a line. The swing ends where the stance begins, and the speeds match there only if the swing has ended at rest and the stance is slow: the foot lands at rest in the air and the ground moves under it, which is why the stance starts with the leg loading rather than sliding. See `E-LEG-0013`.

### Speed and acceleration

The peak speed was 0.87 m/s and the peak acceleration 19.2 m/s2. Doubling the swing time halves the speed and quarters the acceleration (the test checks both). So a slow robot has gentle steps and a fast one has violent ones. It cleared an obstacle 0.04 m high in the middle of the step by 10 mm: the height h is a clearance you choose and pay for in speed.

### What the servos are asked

The foot's path, through the inverse kinematics (34-01) at points along it, gives the joints' angles, and their differences the rates. Over a swing of 0.25 s the three joints needed up to 4.7 rad/s; over 0.125 s, 9.4 rad/s, which is more than the 8 rad/s that a hobby servo can give (typical): the rate is inversely proportional to the time. A step the servos cannot follow becomes a lagging, sagging step, and a place not reachable along the path makes the function say false. See `E-LEG-0014`.

## Build It

`swing_position` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Along the line from p0 to p1 by s = tau cubed (10 - 15 tau + 6 tau squared), and h in height on top: 64 h tau cubed (1 - tau) cubed added to z.

As shipped, all 4 of the 4 tests fail. They are, in the order the suite runs them:

- The swing starts and ends at rest, goes from the leaving place to the landing place, and tops out at the step's height.
- The peaks of speed and acceleration fall as the swing is given more time, and the stance goes back at the body's speed.
- The servos have to turn fast enough, and a shorter swing asks for more.
- A step to the side is treated as a step forward is.

```
rcpp verify 34-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/legged/foot_path.hpp`.

```cpp
double p[3];  rc::legged::swing_position(p0, p1, 0.05, tau, p);
```

**What this does not do.** The swing is in time as a fraction, with the swing time given; a real controller changes it with speed and with what the foot touches. The obstacle is a number: nothing here senses the ground or plans over it (35-08). The height is above the straight line between the two places, not above the ground. The joint accelerations and torques (34-06) are not checked, only the rates. The servo rate of 8 rad/s is typical.

## What Breaks First

- **A swing that goes straight up and down.** See `E-LEG-0012`.
- **A landing with speed.** See `E-LEG-0013`.
- **A step faster than the servos.** See `E-LEG-0014`.

## Ship It

`rc/legged/foot_path.hpp` holds `swing_position`, `stance_position`, `swing_peaks` and `swing_peak_joint_rate`.
