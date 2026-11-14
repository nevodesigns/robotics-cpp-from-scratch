# The Same Program, Another Simulator: One Backend Interface, the Built In Model Behind It, and Gazebo Sim Behind It Too

> A program that only runs on the simulator it was written against has not been tested, it has been rehearsed.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 22-07, 22-12, 22-13, 21-10

## The Problem

A program that drives a robot has been tested against whatever it was driven against. If that was one simulator, what has been learnt is how the program behaves in **that** simulator, including everything that simulator happens to do. The cure is not a better simulator. It is a second one, behind the same door, so that the program can be asked the same question twice and the answers compared. Where they agree there is some reason to believe the program. Where they do not, one of them has told you something.

This lesson makes the door: a small interface, `Backend`, that is everything a driving program needs of a simulator, with the built in model behind it, and (where Gazebo Sim is installed) Gazebo behind it too.

## The Concept

### Four functions

A `Backend` can be **started** on a scenario and a robot, given a **command** (a speed and a turn rate, held until the next), told to **advance** by a time, and asked for the **pose**. That is all a program needs and it hides everything else: how the robot is simulated, in what process, in what units of time. `run_script` is the program of this lesson, the smallest one that is worth the name: it works through a list of (speed, turn rate, seconds) steps and returns the trace of poses, one every so often. It takes a `Backend&` and cannot tell which it has.

### The built in backend

`BuiltinBackend` is the fake drive of lesson 22-01 with the **limits of the robot file** (lesson 22-12: 0.22 m/s and 2.84 rad/s) and a time constant, started at the scenario's start pose. Driving straight for 5 s at 0.2 m/s from a start at y = 2, facing north, ended at y = **2.990**: the 1 m that speed times time says, less the lag, which is the time constant (0.05 s) times the speed. A quarter turn in place ended at **88.8 degrees**: the script is cut into samples of 0.1 s, so the 1.5708 s of the turn became 1.6 s, and the lag took 0.05 rad back off it. A command of 5 m/s was held to **0.22 m/s** by the file's limit. This backend has no terrain and no walls yet; the lessons that follow give it them.

### The Gazebo backend

`rc::gz::GzBackend`, in `rc/gz/gz_backend.hpp`, is the same four functions over **gz-transport**. It is given to you, and worth reading, because most of it is not about robots. On `start` it writes the world of lesson 22-13 to a file, starts `gz sim -s` on it as a child process (headless, and **paused**), puts the child and itself in a partition of their own so that two runs never hear each other, and waits for the robot's pose to appear. `command` publishes a `Twist` on `/model/<name>/cmd_vel`. `advance` asks the world control service for a number of 1 ms steps, and waits until the simulated clock has got there. **The run is as long as it was asked to be, not as long as the computer took**, which is the only way two simulators can be compared. On destruction the server is stopped, and the kernel stops it if the program dies first.

It is built only where Gazebo's libraries are installed, and only on Linux. Where they are not, the lesson builds and its Gazebo test is not there, and every other test passes: that is the rule of the course (a lesson passes without what it cannot assume). See `E-FIELD-0042`.

### The same script on both

The program of the test drives north for 5 s at 0.2 m/s, turns in place for a quarter of a turn at 1 rad/s, and drives on for 3 s. On the built in backend it ended at (0.410, 2.983); on Gazebo Sim, through the same `run_script`, at (0.425, 2.887), **0.098 m** apart and **8.0 degrees** apart in heading, in the same 97 samples. They are in the same neighbourhood and they are not the same robot: the built in model has no terrain, no walls and no rocking body, and Gazebo's wheels answer a command in their own time. That gap is the subject of the next lesson, which measures it case by case.

## Build It

`BuiltinBackend::advance` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each millisecond, give the fake drive the held command again (so that its watchdog stays quiet), step it by 0.001 and add 0.001 to the time.

As shipped, 3 of the 5 tests fail, and these are the ones your code has to turn green:

- A straight drive goes where speed times time, less the lag, says, along the heading.
- A quarter turn in place turns a quarter of a turn, and the limits of the file hold a command back.
- The program is written against Backend and is the same program for any of them.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 22-14
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/field/backend.hpp` opens the module `rc::field`; `rc/gz/gz_backend.hpp` opens `rc::gz`.

```cpp
rc::field::BuiltinBackend builtin;
rc::gz::GzBackend gazebo;               // only where Gazebo is installed
for (rc::field::Backend* b : {static_cast<rc::field::Backend*>(&builtin), static_cast<rc::field::Backend*>(&gazebo)}) {
  b->start(scenario, robot);
  const auto trace = rc::field::run_script(*b, script, 0.1);
}
```

**What this does not do.** Only a speed and a turn rate go in and only a pose comes out: no sensors, no contact, no way to place a second robot or to reset without starting again. Each `start` loads a world, which takes seconds. The Gazebo backend starts a process with `fork`, so it is for Linux; on Windows it is not built. And the two backends are not yet the same robot: the built in one has no friction, no terrain and no walls, and the next lesson measures how far apart they are.

## What Breaks First

- **A program tested on one simulator.** See `E-FIELD-0042`.
- **A comparison of runs of different lengths.** See `E-FIELD-0043`.
- **A command that has not arrived when the world is stepped.** See `E-FIELD-0044`.

## Ship It

`rc/field/backend.hpp` holds `BackendPose`, `Backend`, `BuiltinBackend`, `ScriptStep`, `run_script` and `angle_between`; `rc/gz/gz_backend.hpp` holds `GzBackend`.
