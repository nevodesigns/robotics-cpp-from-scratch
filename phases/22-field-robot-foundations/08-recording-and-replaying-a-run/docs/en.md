# Recording and Replaying a Field Run: A Text Log That Carries Its Scenario, Survives a Cut Power Line, and Proves That Two Runs Were the Same

> A run you cannot replay is an anecdote, and a log that cannot be read after the battery died is not a log.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 22-06, 22-07, 21-10

## The Problem

Something went wrong on the field at 41 seconds, and the only way to learn what is to look at what the robot knew and did in the seconds before it. That is a log. A log that is good enough has three properties that are easy to lose: it can be **replayed** (so a person, or a program, can step through it), it **says what it was** (the scenario it ran travels with it), and it **survives the failure it was written to explain**, which often includes the loss of power in the middle of a write.

## The Concept

### The format

```
runlog 1
| scenario loop
| duration 10
| start 0 0 0
@ 0.02 pose 0.00192 0 0.00115
@ 0.02 gnss 0.00481 -0.0123
```

A version line. The scenario that was run, each line after `| `, so the log is a complete record of the question and the answer. Then the entries: a time, a channel name and the values. Text, so it can be read with an eye, searched with `grep` and compared with `diff`; and in time order, so it can be read as it is written. A ten second run at 50 Hz with two channels was **1000 entries and 72.7 kB**, a small cost for a record that makes the run repeatable.

### Digits

A double needs 17 digits to be given back exactly, and `text()` writes 17. The round trip of the ten second run was identical, bit for bit. The same log written with 6 digits (the default of an unthinking `std::cout`) came back off by up to **0.0000050** in a value, which is small and which is also the difference between two logs of the same run that cannot be told apart by comparing and a bug that cannot be found. See `E-FIELD-0023`.

### Two runs, one difference

`first_difference` gives the time at which two logs first disagree by more than a tolerance. The same seed twice gave none, which is determinism checked rather than hoped for. A different seed gave a difference from **0.02 s**, the first sample, because the receiver's noise is different from the start. When a run changes and nobody knows why, this is where to start. See `E-FIELD-0024`.

### Finding things

`between(t0, t1, channel)` returns the entries in a window and `at(channel, t)` the latest at or before a time, which is what a sensor that holds its last value would show: the pose at 3.005 s was the sample at 3.00. These are the two operations a replay needs.

### A power cut

A robot that loses power loses its last write halfway. The writer ends every line with a newline, so a text that does **not** end in one has a last line that is a stump. `parse_log` drops it, sets `truncated`, and reads the rest: a log cut in the middle of a line gave **999 entries**, one fewer than were recorded, and still parsed. A line that is wrong in the middle, a non-number, a time that goes backwards, stops the read and gives its **line number**. See `E-FIELD-0025`.

## Build It

`parse_log` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Drop an unfinished last line (and set truncated), check the first line, collect the scenario lines, read each entry with its time and values, and stop with fail() at the first line that is wrong.

As shipped, 2 of the 4 tests fail, and these are the ones your code has to turn green:

- A recorded run reads back exactly, scenario and all, and with too few digits it does not.
- A log cut by a power failure still reads, and a bad line stops the read with its number.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 22-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/field/run_log.hpp` opens the module `rc::field`.

```cpp
rc::field::RunLog log;
log.scenario = scenario_text;
log.add(t, "pose", {x, y, theta});
write_file("run.log", log.text());
const rc::field::LogParse p = rc::field::parse_log(read_file("run.log"));
```

**What this does not do.** It holds the whole log in memory, so an hour at 1 kHz is a problem; a long run needs a writer that flushes to disk as it goes and a reader that streams. It has no compression, no checksum (a flipped digit in the middle is read as a number), and no schema for the channels: nothing says that `pose` has three values. And replay here means reading the data back, not driving a robot's code with it, which the next lessons add.

## What Breaks First

- **A log written with too few digits.** See `E-FIELD-0023`.
- **A run that cannot be compared with another.** See `E-FIELD-0024`.
- **A log that is lost when the power is.** See `E-FIELD-0025`.

## Ship It

`rc/field/run_log.hpp` holds `LogEntry`, `RunLog`, `LogParse`, `parse_log` and `first_difference`.
