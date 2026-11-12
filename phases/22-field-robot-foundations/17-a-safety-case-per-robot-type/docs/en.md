# A Safety Case Per Robot Type: Starting Hazards for a Ground Vehicle, an Arm and a Legged Robot, and a Checker That Refuses a Case With a Hole in It

> A safety case is not a document that says the robot is safe, it is a table in which every hazard has a control and every control has a test.

**Type:** Capstone
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-10, 22-11, 22-16, 19-14, 21-10

## The Problem

Every document that says a robot is safe is true until the day it is not. What makes a safety claim worth anything is not the sentence but the **structure behind it**: that every way the robot can hurt someone was written down, that each has something between the robot and the harm, and that someone has made that something fail on purpose to see that it works. A safety case in which any of those is missing is a document with a hole, and the hole is where the accident happens.

Phase 19 built the pieces (a supervisor, a stop path, a geofence, a risk assessment) for one robot at a time. This lesson, which ends the shared base, makes the **table**, for the three robots of the courses that follow, in a form a program can check.

## The Concept

### A case is data

A `SafetyCase` is a robot, a list of hazards, and a list of controls. A **hazard** has a severity (1 slight, 2 serious, 3 death or lasting harm), an exposure (1 rare, 2 frequent) and an avoidability (1 a person can get out of the way, 2 they cannot); the product of the three, 1 to 12, is a **risk level** of low, medium or high (the idea of a risk graph, not the tables of a standard, which a real case must use). A **control** is against one hazard, is of a kind (**design**, which removes the hazard; **protective**, a safety function that stops it; **information**, a sign or a manual), names the **primitive** that implements it if one of the course does, and names the **test** that proves it and whether that test passes.

### A template per robot type

A blank page is how hazards get left out. `template_for` starts a case with the hazards of the kind of robot: **8 for a ground vehicle** (a collision with a person, a runaway after losing the link, tipping, not stopping on a slope, the battery, a trapped limb, a lost position, motion during maintenance), **7 for an arm** (crushing, an unexpected move at power-up, a dropped payload, a singularity, a strike in the workspace, a trapped finger, a fall when power is lost) and **6 for a legged robot** (falling on a person, a hot servo, a trapped finger, thrashing after a fall, the battery, a stumble). Each has several that are high. A case is made by adding to these and removing from them with a reason, not by remembering.

### The checker refuses a hole

`check_case` returns every hole in words. The empty template of the ground vehicle had **16** problems, two for each hazard (it has no control and no stated residual risk). A case with a protective control for each hazard, each with a primitive of the course and a passing test, and a stated residual risk, had **0**. Then five deliberate holes were made: a control no test proves, a test that does not pass, a primitive that does not exist, a high risk with only a sign against it, and a high residual risk nobody signed. With a sixth, a control defined twice, they came to **6** problems, each found, each in words that name the id. One more: a control against a hazard that does not exist. See `E-FIELD-0051`, `E-FIELD-0052` and `E-FIELD-0053`.

### What a control without a test is

A control with no test is a hope. A control whose test does not pass is a known failure. A control that names a primitive that does not exist is a claim about code that is not there. The checker treats all three as holes, and it checks the last against the list of primitives the course built, so that "the supervisor stops it" cannot be written about a supervisor that is not in the library.

### The report

`to_markdown` writes the case as a table, a row for each hazard with its risk, its controls and their tests, and its residual risk, and under it the number of open problems and each one. The report of the arm, filled, was **13 lines** and said **0 open problems**; of the empty template, `none` in every row of controls. It is the document a reviewer reads, and it is made from the data, so it cannot disagree with it. See `E-FIELD-0053`.

## Build It

`check_case` in `exercise/solution.hpp`.

```
rcpp verify 22-17
```

## Use It

`rc/field/safety_case.hpp` opens nothing new in `rc::field`.

```cpp
rc::field::SafetyCase c = rc::field::template_for(rc::field::RobotKind::ground);
c.robot = "yard rover";
// add controls, name their tests, state the residual risks
for (const std::string& hole : rc::field::check_case(c)) std::cout << hole << "\\n";
write_file("safety-case.md", rc::field::to_markdown(c));
```

**What this does not do.** It is not a safety case in the sense of a standard: the risk levels are a teaching scale, there is no argument structure behind the table, no list of the functional safety level each control must reach, and no process for who may change what. It cannot tell that a hazard is missing from the template; the template is a start, and a robot with something the template does not know about has a hazard nobody wrote down. A test that passes is not a test that is good. The checker finds the holes in a table, and a table with no holes is not the same as a safe robot. That is what the review, the hardware section and a qualified person are for.

## What Breaks First

- **A control that nothing proves.** See `E-FIELD-0051`.
- **A high risk controlled by a sign.** See `E-FIELD-0052`.
- **A report written by hand.** See `E-FIELD-0053`.

## Ship It

`rc/field/safety_case.hpp` holds `RobotKind`, `CaseHazard`, `CaseControl`, `SafetyCase`, `risk_level`, `risk_of`, `known_primitives`, `make_hazard`, `template_for`, `is_known_primitive`, `check_case` and `to_markdown`.

This ends phase 22. The tracks that follow start from this base: its interfaces and fakes, its frames, limits, terrain and sensors, its scenario file, log, picture and score, its robot file and its two simulators, and its bring-up and its safety case.
