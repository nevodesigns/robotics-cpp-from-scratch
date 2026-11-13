# Choosing the Robot for the Job: A Task Written as Numbers, Each Family Checked Against It, and the Fastest One That Passes Everything

> The right robot is found by writing the task down and letting each candidate fail it, one reason at a time.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 32-05, 32-04, 31-07, 31-06, 21-03

## The Problem

32-05 put the five robots on one yardstick and said the **task** chooses, not the volume. This lesson is the **capstone of the arm course**: write a task as **numbers**, let each family be checked against it, and see which **fails for what reason**. The method is the point; the figures for each family are **typical** and a real choice uses a data sheet and a trial.

## The Concept

### A task as numbers

The area of the work (a rectangle of half widths), the **payload**, whether the tool must **tilt**, the **accuracy** needed, the **cycle** wanted, the lengths of the short and the long moves, and the time the **hand** takes (a vacuum cup takes a fifth of a pinching hand's time, 31-01). It is a small struct, and **writing it down is most of the work**: a task described in words ("pick and place, fast") fits every robot.

### Each family fails for a reason

For each family the check says **which requirement it fails**:

- **Area**: a grid of 63 points over the work is put to the family's reach test (32-05), and it fails if any point is out of reach. A SCARA mounted **over the middle** of the work reached **0.48** of it (it cannot reach under itself, 32-01) and mounted **beside** it **1.00**; on a wide table the delta reached **0.33**, the Stewart platform **0.05** and the arm **0.89**. See `E-ARM-0118`.
- **Payload**, **tilt** and **accuracy**: a comparison of the task with the family's typical figures. A delta and a SCARA **cannot tilt** (32-01, 32-02); a Stewart platform is accurate to hundredths of a millimetre and slow.
- **Cycle**: the cycle time of 31-07, from the family's speed and acceleration and the task's moves and hand time, against the cycle wanted. See `E-ARM-0119`.

### The fastest that passes everything

Of the families that fail **nothing**, the one with the **shortest cycle**. Four jobs went to **four different robots**:

- small light parts, fast, with a vacuum cup: family **1** (the delta, a cycle of **0.81 s**);
- a heavier part turned over and inserted: family **4** (the six axis arm, **2.71 s**), since it needs a tilt and a payload that only the arm and the Stewart platform allow, and the arm's reach is the larger;
- optics held and tilted to a hundredth of a millimetre: family **2** (the Stewart platform, **2.0 s**), which is the only one with the accuracy and the payload;
- a wide table of boxes, level: family **3** (the gantry, **2.82 s**), the only one whose reach covers it.

A job of **500 kg** gave **-1**: **no robot**, and saying so is a result. See `E-ARM-0120`.

## Build It

`choose_family` in `exercise/solution.hpp`.

```
rcpp verify 32-06
```

## Use It

`rc/arm/cell.hpp`.

```cpp
rc::arm::Robots robots;  rc::arm::FamilyData fam[5];  rc::arm::typical_families(fam);
rc::arm::Verdict v[5];
const int pick = rc::arm::choose_family(robots, fam, 5, task, v);
```

**What this does not do.** **Every figure for a family is typical**, and a real robot of a family can differ from it by a large factor; the list has **one design for each family**. The **cycle** is the pick and place of 31-07 with a straight move of two lengths; **people near the cell** (31-06), the **camera and its calibration** (31-03), **retries** (31-02) and the **cost, floor space and life** of the cell are not counted. The **area check is a grid at one height** with the robot mounted at a **set offset**; a real cell tries more mountings. The choice is **one number, the shortest cycle**, among those that pass: a person might prefer cost, which is not in the struct.

## What Breaks First

- **A robot chosen before the task is written as numbers.** See `E-ARM-0118`.
- **A cycle worked out with the wrong hand.** See `E-ARM-0119`.
- **A choice that cannot say none.** See `E-ARM-0120`.

## Ship It

`rc/arm/cell.hpp` holds `TaskSpec`, `FamilyData`, `Verdict`, `Robots`, `typical_families`, `check_family` and `choose_family`. With it phase 32 is complete, and so is the arm course: an arm that knows its geometry, dynamics and the space it works in, plans a motion that touches nothing, picks and places, finds the part with a camera whose place it has measured, feels contact, inserts with give, keeps its distance from a person, can be led by hand; and a choice of kind of robot made from the task.
