# Six Faults and a Matrix: Which Check Catches Which Fault, and the Faults That Only One Check Sees

> A green suite says every check passed. It does not say any of them could have failed. The way to find out is
> to put the faults in on purpose and draw a table.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-08, 05-03, 05-08

## The Problem

Lesson 05-03 had a test catch a bug. Lessons 05-08 to 05-11 asked what to assert, how wide to make it, whether to check state or calls, and how to compare a log. A suite built on all of that is green. What it has not yet shown is that it would go red for the faults that matter.

The way to show it is the way a fire alarm is tested: start a small fire. Put each fault into the loop in turn, run every check, and write down which ones fail.

## The Concept

### Six faults, three checks

The loop is a first-order plant under PI control, as in lesson 05-08. Each fault breaks one thing:

| fault | what is wrong |
|---|---|
| sign flipped | the controller gains are negative |
| sensor offset | the sensor reads 0.2 too high |
| weak actuator | the actuator can deliver only 0.8, though the controller asks for up to 2.5 |
| limiter removed | the controller no longer holds its output to the limit |
| measurement late | the controller sees the sensor 0.7 s late |
| sensor dropout | the sensor reads zero for 0.6 s |

Three checks, each returning true for a good run:

1. **response meets spec**: overshoot, settling time and steady error, from lesson 05-08;
2. **output within limit**: the controller never commands more than the specification allows;
3. **sensor agrees with model**: what the controller was told matches a model of the plant driven by what it commanded.

### The table

`detection_matrix` runs every check on every faulty run and records a fault as caught when a check fails:

| fault | response meets spec | output within limit | sensor agrees with model |
|---|---|---|---|
| sign flipped | CAUGHT | . | . |
| sensor offset | CAUGHT | . | CAUGHT |
| weak actuator | CAUGHT | . | . |
| limiter removed | . | CAUGHT | . |
| measurement late | CAUGHT | . | CAUGHT |
| sensor dropout | CAUGHT | . | CAUGHT |

The clean run passed all three, so there are **0** false alarms. **0** faults got past every check. See `E-TEST-0030`.

### The faults with one witness

Three faults are seen by exactly one check: the flipped sign and the weak actuator by the response check, the removed limiter by the limit check. Take a check away and see what is lost:

| check removed | faults no longer caught |
|---|---|
| response meets spec | 2 |
| output within limit | 1 |
| sensor agrees with model | 0 |

The third check adds nothing to the count: on every fault it sees, the response check sees it too. What it adds is the reason, the sensor disagreeing with the plant. A check that never changes the count is not useless, but it should be kept on purpose. See `E-TEST-0031`.

### Loosening

Change the response check to "the response ends within 0.6 of the target". The weak actuator ends 0.2 away, so it is accepted, and the other two checks do not see it. It is **missed by every check**, and **4** of the 6 faults are now seen by exactly one check, where there were 3. Re-run the table after every change to a bound. See `E-TEST-0032`.

## Build It

`detection_matrix`, `false_alarms`, `faults_missed`, `faults_with_one_witness` and `unseen_without` in `exercise/solution.hpp`. The faults, the loop and the checks' type are given.

```
rcpp verify 05-12
```

## Use It

`rc/test/matrix.hpp` joins `rc::test`.

```cpp
const std::vector<std::vector<bool> > caught = detection_matrix(checks, runs);
RC_CHECK(faults_missed(caught).empty());
RC_CHECK_EQ(false_alarms(checks, clean_run), 0);
```

**What this does not do.** It shows that the faults you thought of are caught, and says nothing about the ones you did not. Six faults in one simple loop are a start and not a proof: a real list grows with every bug that reaches a robot. It treats each fault as all or nothing, whereas a real fault has a size, and a check that sees an offset of 0.2 may not see
0.02 (lesson 05-09 is about that). And the table has to be run, which costs a simulation per fault, so it is a test for the suite and not for every build.

## What Breaks First

- **A suite never shown to catch anything.** A fault goes through. See `E-TEST-0030`.
- **A fault with one witness.** Delete the check and it is gone. See `E-TEST-0031`.
- **A check loosened without redrawing the table.** See `E-TEST-0032`.

## Ship It

`rc/test/matrix.hpp` holds `Fault`, `Run`, `simulate_fault`, `Check`, `detection_matrix`, `false_alarms`, `faults_missed`, `faults_with_one_witness` and `unseen_without`.

That is phase 05's ending in full: a test suite you can prove catches bugs, because you watched it catch six, and a table that says which of its checks you cannot afford to lose.
