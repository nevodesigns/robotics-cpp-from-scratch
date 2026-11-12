# A Test Report You Can Read: Scenarios Tied to Hazards, a Table That Names What Is Not Covered, and a Regression It Catches

> A test suite is not evidence of safety. A table that says which hazard each test is evidence for, and which have none, is.

**Type:** Capstone
**Time:** about 140 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 21-07, 21-02, 19-14, 19-01, 18-11

## The Problem

The robots of the earlier capstones each came with tests, and each test said "this passed". A reader of a project does not want to know that 23 tests passed. They want to know **which of the things that can go wrong have been tried**, and which have not. The hazard log of lesson 19-01 is that list, and the risk assessment of 19-14 says what stops each. What is missing is the line between the two and the tests: for each hazard, **the scenario that is evidence for it**.

This lesson builds that line and the report it produces: the thing to hand to somebody who is deciding whether to trust the robot. It uses the rover of 21-02 as the thing under test.

## The Concept

### A scenario is evidence for hazards

A `SuiteScenario` has an id, a title, the configuration of the rover, what is put in the building, **the hazard ids it is evidence for**, and a rule for passing. The five of this suite:

| scenario | for hazards | passes if |
|---|---|---|
| S1 an empty building | none | reached, no contact, a gap of more than 0.1 m |
| S2 somebody steps into the path | G1, G4 | reached, no contact, gap more than 0.1 m |
| S3 somebody in the doorway, then gone | none | reached, no contact |
| S4 the link lost for four seconds | G2 | not reached, stopped with cause link, still at the end |
| S5 the range sensor dies | G8 | not reached, stopped with cause sensors, no contact |

Note what "passes" means for S4 and S5: **not reaching the goal is the pass**. A safety scenario that the robot passes by completing the task is not testing safety. And S1 and S3 are not evidence for any hazard of the log; they are there so that the rover is not "safe" by never moving.

### Coverage

`hazard_coverage` goes through the hazards of class **medium or worse** (by the risk class of 19-01; a low one is left out) and gives each a status: **covered** if at least one scenario names it and all of them pass, **failing** if one of them fails, **uncovered** if none names it. With all five passing, the eight hazards of the log:

- covered: **G1** and **G4** (by S2), **G2** (by S4), **G8** (by S5)
- uncovered: **G3** (rolls over on a slope), **G5** (battery fails), **G6** (motor overheats), **G7** (leaves the area)

**Four of the eight have no test at all**, and that goes in the report. This is not a fault of the report; it is **its purpose**. 21-02's rover has no tilt sensor, no battery model and no fence in its scenarios, and a report that said "all tests pass" would hide that. See `E-CAP-0022`.

### The report

A fixed layout, so that it can be compared between runs and read by people who have read it before:

```
Scenarios
S1  PASS  an empty building                   reached 12.3 s, 0 bumps, closest 0.18 m
S2  PASS  somebody steps into the path        reached 22.6 s, 0 bumps, closest 0.16 m
S3  PASS  somebody in the doorway, then gone  reached 27.2 s, 0 bumps, closest 0.34 m
S4  PASS  the link lost for four seconds      not reached, 0 bumps, stopped: link
S5  PASS  the range sensor dies               not reached, 0 bumps, stopped: sensors

Hazards of class medium or worse
G1  high          runs into a person                            covered by S2
G2  unacceptable  keeps driving when the link is lost           covered by S4
G3  medium        rolls over on a slope                         NOT COVERED
G4  high          cannot stop in the distance its sensor sees   covered by S2
G5  medium        battery fails in the field                    NOT COVERED
G6  medium        motor overheats and burns                     NOT COVERED
G7  medium        leaves the area it is allowed in              NOT COVERED
G8  high          a sensor fails and nothing notices            covered by S5

Summary: 5 of 5 scenarios passed; 4 of 8 hazards covered by passing scenarios; not covered: G3, G5, G6, G7
```

Every number in it came out of the run. The layout was tested separately, with results made by hand (the test checks the text to the character, including the columns), so that a change to the **physics** cannot break the **report**, and a change to the report is a change somebody has to look at.

### A regression it catches

A change that takes the range limiter and the supervisor out of the rover. Run the suite:

| scenario | with the safety layer | with it taken out |
|---|---|---|
| S1 an empty building | PASS | **PASS** |
| S2 somebody steps into the path | PASS | **FAIL** |
| S3 the doorway | PASS | **PASS** |
| S4 the link lost | PASS | **FAIL** |
| S5 the sensor dies | PASS | **FAIL** |

The unsafe rover still drives through the empty building, as the lesson on the line follower said any robot does: **the functional tests pass**, and the three safety scenarios fail. The summary line then reads `failing: G1, G2, G4, G8`, with `suite_green` false. A robot whose only tests are the functional ones has no way to know. See `E-CAP-0023`.

### The gap that is green

And the other direction. A suite with the link scenario **left out**: every scenario passes and the suite is **green**, and the report says `G2  unacceptable  ...  NOT COVERED`, with G2 first in `not covered: G2, G3, G5, G6, G7`. An uncovered hazard is not a failure of the tests that were written, and the report does not call it one: but it is the line a reviewer reads first, and the **unacceptable** one is the one that must never be there. See `E-CAP-0024`.

## Build It

`hazard_coverage`, `render_report` and `suite_green` in `exercise/solution.hpp`. The scenarios, the building and the runner are given.

```
rcpp verify 21-08
```

## Use It

`rc/capstone/test_report.hpp` joins `rc::capstone`.

```cpp
const std::vector<SuiteRow> rows = run_suite(scenarios);
const std::vector<HazardLine> coverage = hazard_coverage(hazard_log, scenarios, rows);
std::cout << render_report(scenarios, rows, coverage);
return suite_green(rows, coverage) ? 0 : 1;
```

in the continuous integration of the project, and the text of the report saved with the build.

## Show It

Put **the report itself** in the README, with the uncovered lines in it. Nothing builds trust in a safety claim like a table that says what has not been tried. Pair it with the regression table: the day somebody removes the limiter, the build goes red with the hazard ids in the message.

**What this does not do.** A scenario is evidence **only if its rule is a good one**: the report counts passing scenarios and cannot tell a test that would catch the hazard from one that cannot. The hazards are those somebody wrote down, and the link between a scenario and a hazard is **a claim** that a person made in the table, not something the code can check: a scenario can name G1 without being any evidence for it. A covered hazard is **tried once, in one building, with one person**, which is a sample and not a proof (lesson 19-13 is about how many it takes). And the report says nothing about **how likely** a hazard is after the mitigation: that is the assessment of lesson 19-14, and the report is evidence for the claims in it, which is its job.

## What Breaks First

- **A report that says only how many tests passed.** See `E-CAP-0022`.
- **Functional tests standing in for safety tests.** See `E-CAP-0023`.
- **A hazard with no scenario, in a green suite.** See `E-CAP-0024`.

## Ship It

`rc/capstone/test_report.hpp` holds `report_building`, `SuiteScenario`, `SuiteRow`, `run_suite`, `CoverageStatus`, `HazardLine`, `hazard_coverage`, `render_report` and `suite_green`.
