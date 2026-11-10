# The Expected Output That Changed Every Run: Comparing a Log With a Saved One, and Showing the Line That Differs

> A robot's log is the easiest thing to test: run it, save the output, and compare. It fails on the second machine
> because every line has a time in it, and when it does fail it says only that something differs.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 01-05, 05-01

## The Problem

A run of the robot writes a log: motor enabled, speed 0.5, speed 1.0, motor disabled. Saved from a good run, that file is a test: compare the next run's log with it. The idea is called a **golden file**, and it is cheap to write.

It passes on the machine where it was saved and fails everywhere else. Each line starts with the time, and the first line has an address in it. When it fails for a real reason it says only that the files differ.

## The Concept

### Replace what is volatile

Two things change from run to run in this log for no reason: the time, written `t=12.345`, and an address, written `0x7ffd5a3c`. `normalize` replaces them with fixed words, `t=<time>` and `0x<addr>`, and leaves every other character alone.

Two runs of the same program, 4 lines each: **4** lines differed as written and **0** after normalising. See `E-TEST-0027`.

### Replace only what is volatile

It is tempting to strip every number. That makes the comparison independent of the time and also of the speed. The rules here are narrow, and tested so:

| line | result |
|---|---|
| `t=12.345 motor enabled` | `t=<time> motor enabled` |
| `buffer at 0x7ffd5a3c ok` | `buffer at 0x<addr> ok` |
| `t=1.5 copy 0xDEADbeef to 0x10` | both replaced |
| `speed 0.5 at limit 1.0` | unchanged: numbers that matter stay |
| `target=3 t= none` | unchanged: `t=` with no number is not a time |
| `0x is not an address` | unchanged |
| `soft=2.5` | unchanged: the `t=` is inside a word |

See `E-TEST-0028`.

### Say which line

A yes or no was all the comparison was asked for. The difference between two logs is a list of lines kept, removed and added, found as the **longest common subsequence**: `table[i][j]` is how many lines the first `i` of one and the first `j` of the other have in common, in order, and walking back from the corner keeps equal lines and otherwise goes the way that keeps more.

With the final speed changed from 1.0 to 1.25:

```
  t=<time> motor enabled at 0x<addr>
  t=<time> speed 0.500000
- t=<time> speed 1.000000
+ t=<time> speed 1.250000
  t=<time> motor disabled
```

Two changes, and the one line is named. On 500 random pairs of short line lists the difference rebuilt both sides every time and was never longer than removing everything and adding everything. A line that changed is one removed and one added. See `E-TEST-0029`.

## Build It

`normalize` and `diff` in `exercise/solution.hpp`. The `DiffLine` type and `changes` are given.

```
rcpp verify 05-11
```

## Use It

`rc/test/golden.hpp` joins `rc::test`.

```cpp
const std::vector<DiffLine> d = diff(normalized(saved), normalized(now));
if (changes(d) != 0) { for (const DiffLine& line : d) std::cout << line.op << " " << line.text << "\n"; }
```

**What this does not do.** It compares whole lines, so a line with one digit changed is a removal and an addition and not a small edit. It does not know about the order of lines that can legitimately vary, such as two threads logging, which needs sorting or a different test. The table is as big as the two logs multiplied together, which is fine for
a few thousand lines and not for a million. A golden file also has to be updated when the behaviour is meant to change, and an update that is approved without reading the difference makes the test pass for the wrong output.

## What Breaks First

- **A saved log with a time in it.** No run matches. See `E-TEST-0027`.
- **Normalising everything.** A real change is hidden. See `E-TEST-0028`.
- **A verdict with no difference.** Slow to read. See `E-TEST-0029`.

## Ship It

`rc/test/golden.hpp` holds `normalize`, `DiffLine`, `diff` and `changes`.

Lesson 05-08 said to assert a behaviour and not a number, and lesson 05-10 the state and not the calls. A golden file is the other end of the scale: the whole output, compared exactly, once the parts that cannot be the same are taken out.
