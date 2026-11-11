# A Script for the Test Day: Timed Events From a Text File, and Delivering Each Exactly Once

> A test that is written in C++ can only be changed by someone who can build C++. A test that is written in a few lines of text can be changed by whoever was at the bench.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-01, 01-05, 03-02, 01-06

## The Problem

The simulator has a plant (lesson 18-02), a model of the plant from measurements (18-03) and a sensor that lies (18-04). A test of the robot is a story told to them: go to 1.0, then at 2.5 s to -0.5, push it at 4 s, break the sensor at 6, mend it at 7. Written as code, each story is a function, and changing a time means a rebuild.

A **scenario file** is that story as a few lines of text: the same file runs against the simulation today and the bench tomorrow, a person who has never built the repository can read it and write the next one, and a bug found on the robot is reproduced by adding lines to a file. Two things have to be right for that to be worth anything: the file must be read carefully, since it is typed by hand,
and each event must happen exactly once, at the step it belongs to.

## The Concept

### A line, an event

```
# a step, a push and a sensor fault
at 0.0 target 1.0
at 2.5 target -0.5   # swing the other way
at 4.0 disturb 3.0
at 6.0 fault sensor stuck
at 7.0 fault sensor clear
at 10 end
```

Each line is `at`, a time, an event and its words. Comments run from a `#` to the end of the line and blank lines are ignored. The parse gave 6 events, lasting 10 s, each with the **line it came from**: the second target was on line 4 of the file, and that number is what an error message about it will say. A script whose last line has no newline reads the same. See `E-SIM-0013`.

### Typed by hand, and wrong

Every kind of mistake a person makes at a keyboard was put in a script of 14 lines, and **every bad line was reported**, with the parse carrying on to find the rest:

```
bad lines found: 2 3 4 5 6 8 9 10 11 12 14
first: line 2: the time is not a number: x
```

| line | the mistake |
|---|---|
| 2 | the time is not a number |
| 3 | an event that does not exist |
| 4 | a target with no number |
| 5 | a target with two |
| 6 | a negative time |
| 8 | a time that goes backwards |
| 9 | a fault that does not say which |
| 10 | an end with something after it |
| 11 | a line that does not start with `at` |
| 12 | a number that is not one (`1.5e`) |
| 14 | an event after the end |

A parser that stops at the first mistake makes a person fix them one run at a time. One that reads **1.5e as 1.5** and carries on, as `std::stod` does with the characters it does not understand, runs a different scenario from the one written. A number here is the whole word or it is an error. And when there is any error, the scenario is left **empty**: a half read script that runs is worse than one that does not. A script with no end, one with two,
and an empty text are all refused too. See `E-SIM-0013`.

### Exactly once

The simulation steps (lesson 18-01) and the scenario has events at times. The step from `t0` to `t1` takes the events with `t0 < time <= t1`: after the start of the step and up to and including its end. Then consecutive steps cover the line without a gap and without an overlap. Stepping 10 ms for 11 s, each of the 6 events was delivered exactly once, at steps 1, 250, 400, 600, 700 and 1000. The event at
2.5 s arrived at step 250, the step that **ends** at 2.5 s, and the next step did not deliver it again.

Both ends of the interval matter. If it included `t0` as well as `t1`, an event on a step boundary would arrive twice. If it excluded both, it would arrive never. And the very first interval starts at zero, so with `t0 = 0` an event **at** time zero, like the first target here, belongs to no interval at all: `events_between(0, 0.01)` gave nothing and `events_between(-1, 0.01)` gave it. The first step must start before zero. See `E-SIM-0014`.

### Together, in order

Events at the same time arrive together in the order they were written: a target, a push and a fault all at 1.0 s came out in that order, and a script can depend on it, such as setting a target before the push that tests it.

### The time of a step

The last trap is in the caller. An event at 1.0 s with steps of 0.1 s:

```
the time of each step built up by adding 0.1 delivers it at step 11, worked out as step number times 0.1 at step 10
```

Adding 0.1 ten times gives 0.9999999999999999, which is not at least 1.0, so the step that should end at 1.0 ends just short, and the event arrives a whole step late. Worked out fresh each step as the step number times the step, ten steps is 1.0 exactly. A scenario is only as precise as the clock it is run against: use the step count and not a running sum. See `E-SIM-0015`.

## Build It

`parse_scenario` and `events_between` in `exercise/solution.hpp`. The event types, `words_of`, `parse_number` and `duration` are given.

```
rcpp verify 18-05
```

Nothing is random.

## Use It

`rc/sim/scenario.hpp` joins `rc::sim`.

```cpp
rc::sim::Scenario scenario;
const rc::sim::ParseResult r = rc::sim::parse_scenario(file_text, scenario);
if (!r.ok) for (const auto& e : r.errors) std::cerr << "line " << e.line << ": " << e.message << "\n";
// every step, with t0 starting at -1 and t1 = steps * dt:
for (const auto& e : rc::sim::events_between(scenario, t0, t1)) apply(e);
```

**What this does not do.** It reads times in seconds and no other unit, and has four events: a real scenario language has loops, named signals and expected results, and a lesson later in this phase gives a scenario something to check. It does not say what a `fault` means: the words are the caller's, and a fault the simulation does not know has to be reported by whoever applies it. It reads the whole text into memory, which is right for a script and wrong for a log. And it trusts the file's
times to be in seconds on the simulation's clock, not the wall clock.

## What Breaks First

- **A parser that stops at the first error, or reads half a number.** Scenarios that are not the ones written. See `E-SIM-0013`.
- **An interval that includes both ends or neither.** Events twice or never, and the one at time zero missed. See `E-SIM-0014`.
- **A step time built up by adding.** Every event on a round number arrives a step late. See `E-SIM-0015`.

## Ship It

`rc/sim/scenario.hpp` holds `ScenarioEvent`, `ScenarioError`, `Scenario`, `ParseResult`, `words_of`, `parse_number`, `parse_scenario` and `events_between`.

A test day can now be written down in a file, read carefully and replayed.
