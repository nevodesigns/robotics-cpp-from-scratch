# A Field Scenario File: One Text File for the Ground, the Walls, the Goals and the Faults, Parsed With Every Mistake Reported at Once

> A test that cannot be written down in a file cannot be repeated, shared, or run in another simulator.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-05, 22-06, 21-10

## The Problem

The robot, the sensors and the ground are code. The thing the robot is asked to do on that ground is a **test**, and a test that exists only as a function in a source file cannot be handed to a colleague, cannot be kept as a record of what was run, and cannot be given to a second simulator. So the test has to be a file: where the ground is soft, where the walls are, where the robot starts and has to get to, and what goes wrong, and when. This lesson writes that file's format and the parser that reads it, with one rule for the parser that it keeps even when it costs effort: **it reports every mistake, not the first.**

## The Concept

### A statement per line

```
# a yard loop
scenario yard_loop
duration 60
seed 7
ground gravel
patch sand 5 0 10 10
patch ice 7 0 8 10 0.1 0
wall -10 -10 10 -10
post 3 2 0.3
start 0 0 90
goal 8 5 0.3
fault gnss_outage 20 30
fault lidar_dropout 10 15 0.5
```

Ten keywords. Lengths are metres, times seconds, angles in the `start` line degrees (a person writes 90, not 1.5708). A `patch` is a surface, a rectangle, and optionally the slope of its plane; a later patch lies over an earlier one, as in lesson 22-05. A `fault` is a kind, a start, an end and, for most kinds, one number: a dropout probability, a slip, a bias. A `#` begins a comment, to the end of the line. The good file of **15 lines** gave **2 patches, 1 wall and 2 faults**.

### Every mistake, with its line

A file with six mistakes, one per line (no value after `scenario`, a duration of `sixty`, a surface called `mud`, a patch one number short, a keyword `wibble`, and a fault that ends before it starts) gave **8 reports**, six for the lines and two for the whole file (no valid duration, no start). A parser that stops at the first mistake makes a person fix, rerun and find the next, six times. One that reports a line number and the word that was wrong makes it one pass. The checks that need the whole file (a start, a duration, a fault inside the run) are made after the last line has been read. See `E-FIELD-0020`.

### What the scenario makes

`terrain_of` makes the `Terrain` of lesson 22-05, `world_of` the walls and posts for the lidar of lesson 22-06, and `fault_active(kind, t)` says which fault is on. These are the same functions for the built in simulator and, in a later lesson, for the world given to Gazebo: one file, two worlds, which is the whole reason it is a file. See `E-FIELD-0021`.

### The round trip

`write_scenario` writes a scenario back in a fixed order, and a file parsed, written, parsed and written again gave the same text both times. That is the test that nothing is lost between the file and the structure, and it is what lets a later tool (the recorder of the next lessons) put the scenario it ran into its own log. See `E-FIELD-0022`.

## Build It

`interpret` in `exercise/solution.hpp`: one branch for each keyword.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- One branch per keyword: check the count of values (count), read the numbers (numbers), check what must hold (a surface that exists, x1 > x0, a positive radius), and fill the scenario or call bad(); an unknown keyword is a problem too.

As shipped, all 5 of the 5 tests fail. They are, in the order the suite runs them:

- A good file is read into the scenario, comments and blank lines and all.
- Every mistake in a bad file is reported once, with the line it is on.
- Writing a scenario and reading it back repeats it exactly.
- The terrain, the lidar world and the faults come out of the scenario.
- A file with no start and no duration says so, and a fault past the end is a problem.

```
rcpp verify 22-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/field/field_scenario.hpp` opens the module `rc::field`.

```cpp
const rc::field::ScenarioParse p = rc::field::parse_scenario(text);
if (!p.ok()) for (const auto& problem : p.problems) report(problem.line, problem.message);
const rc::field::Terrain terrain = rc::field::terrain_of(p.scenario);
```

**What this does not do.** It has no includes, no loops and no expressions, so a hundred posts is a hundred lines (a generator writes them). The faults are windows in time, not triggered by an event. Numbers are read with the C locale rules, one dot, no comma. And it does not check that the start is not inside a wall, or that a goal can be reached: the scenario says what is asked, not whether it can be done.

## What Breaks First

- **A parser that stops at the first mistake.** See `E-FIELD-0020`.
- **A scenario that lives in the source.** See `E-FIELD-0021`.
- **A writer that does not round trip.** See `E-FIELD-0022`.

## Ship It

`rc/field/field_scenario.hpp` holds `FieldPatch`, `FieldGoal`, `FieldFault`, `FieldScenario`, `ScenarioProblem`, `ScenarioParse`, `parse_double`, `surface_by_name`, `fault_params`, `split_words`, `interpret`, `validate`, `parse_scenario`, `write_scenario`, `terrain_of`, `world_of` and `fault_active`.
