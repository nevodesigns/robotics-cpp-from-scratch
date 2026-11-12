# A Risk Assessment You Can Defend: Every Serious Hazard Tied to a Mechanism That Exists and Evidence That Passes, With the Residual Risk Worked Out

> A risk assessment is defensible when someone who wants to find a hole has to work, and cannot find one that the assessment has not already admitted to.

**Type:** Capstone
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-13, 19-12, 19-11, 19-10, 19-01

## The Problem

Lesson 19-01 wrote down eight hazards of a ground vehicle and scored them: one unacceptable, three high, four medium, all open. Thirteen lessons later there is a stop path, fail-safe outputs, a speed from range, a fence, a cross-check, a power monitor, a tilt monitor, a supervisor, a self-test, a black box and the arithmetic of residual risk. What is missing is the document that says **which of those answers which hazard**, and shows that the answer holds.

The usual document is a spreadsheet: a column of hazards, a column of "mitigations" and a column of residual risk that someone typed. It fails in three ways. The mitigation is a **promise** (a name with nothing behind it). The mitigation **was true once** (the test was run before the last change). And the residual risk is a **number nobody can reproduce**. A reviewer who asks "show me" gets a pause. This lesson builds an assessment that **can be run**: it checks its own claims, and says exactly why it fails when it does.

## The Concept

### An entry: a hazard, a rate, and what is done

An `Entry` holds a hazard from 19-01 (severity, likelihood, owner), the **demands per year** (how often the situation arises), a list of `Mitigation`s and the name of the person who **accepted** what is left. A `Mitigation` names:

- the **mechanism** that does it (`rc::safety::StopPath`), which has to be in the set of mechanisms that exist;
- its **group** (software, hardware, procedure): things in one group can fail for the same reason;
- its **pfd**, the chance it is dead when it is needed (lesson 19-13);
- its **evidence**: a function that exercises the mechanism and returns whether it did its job.

The evidence is code. The evidence for G2's stop path makes a `StopPath`, feeds it heartbeats, stops them, and checks that the output goes to zero and stays there. The evidence for the brake commands the coil to zero and cuts its wire. All of the evidence in the tests runs against the real `rc::safety` headers, so it is the same code that lessons 19-02 to 19-12 tested: if somebody changes `StopPath` in a way that breaks it, the assessment stops being defensible at the next run.

### Credit only for what is proven

`assess` goes through the entries in order. A mitigation gets **credit** only if its mechanism is known **and** its evidence passes: a mitigation that fails either is a finding (`unknown mechanism: ...`, `evidence failed: ...`) and counts for nothing. A mitigation that is a name with no evidence function fails. This is the whole difference between a document and an assessment: **a claim with nothing under it is worth zero**, and not a smaller number.

### Redundancy counts only across groups

Two mitigations in the same group do not help each other: they can fail for the same reason. So within a group only the best counts, and across groups they are redundant with a common-cause share beta: the product over the groups of `(1 - beta) pfd`, plus `beta` times the best pfd (lesson 19-13). A software function at 3.6e-03 alone gave **3.60e-03**, **two in the same group gave 3.60e-03 again**, a software function and a hardware brake at 1e-4 with beta 0.10 gave **1.03e-05**, and with beta 0 gave **3.60e-07**. A pfd that is not a number counts as 1.

### The residual risk, worked out

The residual frequency is the demands times the combined pfd, and its likelihood class and the severity give the residual class, by the matrix of lesson 19-01 with its floor for a catastrophe. For the eight hazards of lesson 19-01, with a software function at 3.641e-03 (a failure rate of 1e-5 per hour tested monthly) and a spring applied brake at 1e-4 (a data sheet figure, an **input**), and beta of 0.10:

| hazard | before | after | pfd | per year | kept |
|---|---|---|---|---|---|
| G1 runs into a person | high | medium | 1.03e-05 | 1.03e-04 | 2 of 2 |
| G2 keeps driving when the link is lost | unacceptable | medium | 1.03e-05 | 2.06e-04 | 2 of 2 |
| G3 rolls over | medium | medium | 3.64e-03 | 1.82e-02 | 2 of 2 |
| G4 cannot stop in its sight line | high | medium | 1.03e-05 | 3.09e-04 | 2 of 2 |
| G5 battery fails | medium | medium | 3.64e-03 | 4.37e-02 | 1 of 1 |
| G6 motor overheats | medium | medium | 3.64e-03 | 2.18e-02 | 1 of 1 |
| G7 leaves its area | medium | medium | 3.64e-03 | 3.64e-02 | 2 of 2 |
| G8 a sensor fails unnoticed | high | medium | 3.64e-03 | 1.46e-02 | 1 of 1 |

**0 findings, defensible: yes.** One unacceptable, three high and four medium became **eight medium**. Look at G3 and G7: two mitigations kept, and the pfd is still **3.64e-03**, the one function's: both are software. Look at G1, G2 and G4: they are medium **only** because of the floor, as in lesson 19-13: severity 4 is never lower, whatever the arithmetic says (a likelihood of 1 with a severity of 4 scores 4, which would be low). And the three of them, being catastrophes, carry the name of a person who accepted what is left. See `E-SAFE-0040`.

### Take a mitigation away

The assessment is useful for what it says when something changes. Take G2's hardware brake away, leaving the software stop path alone:

- `G2: a catastrophe needs two independent groups`
- `G2: residual risk is high or worse` (7.28e-02 per year, class **high**)

Change the brake to one that is **applied by power** (lesson 19-03), and the evidence for the brake fails in **G1, G2 and G4**, where it was relied on: **3 evidence failures, 9 findings in all**, each hazard losing its credit for the brake, its second group and its residual class together. The change is in the one line of the design that says which brake, and the assessment follows it to everything that depended on it. See `E-SAFE-0041`.

### Six ways to cheat, caught

| change | caught as |
|---|---|
| G2's stop path is a mechanism called "magic watchdog v2" | `unknown mechanism: magic watchdog v2` |
| G1's acceptor is left blank | `residual risk of a catastrophe is not accepted by name` |
| a ninth hazard, a pinch point, with nothing done about it | `no mitigation` |
| G5's pfd is a NaN | `residual risk is high or worse` (it is counted as 1) |
| a second hazard with the id G1 | `hazard not accepted into the log` |
| a second software mitigation for G7 | no credit: its pfd was **3.64e-03** before and after |

**6 of 6** were caught in the way that they should be. The last one is the quiet one: it is not an error, but the new mitigation earns nothing, which is the point. See `E-SAFE-0042`.

## Build It

`combined_pfd` and `assess` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 19-14
```

## Use It

`rc/safety/assessment.hpp` is the last header of `rc::safety`.

```cpp
const rc::safety::Report r = rc::safety::assess(entries, known_mechanisms, 0.10);
for (const auto& f : r.findings) std::cerr << f.hazard << ": " << f.problem << "\n";
return r.defensible ? 0 : 1;          // as a test in the project's continuous integration
```

Run it in the continuous integration of the robot, with the evidence as real tests of the real code, and a failing build is the first notice of a change that broke a safety claim.

**What this does not do.** It is a **tool for keeping an assessment honest, not a certificate**. It does not find hazards: lesson 19-01's list is as good as the people who made it, and **a hazard that was never written down is not in the report**. It does not check that the evidence is a good test of the mechanism, only that it runs and passes; a bad test passes. The pfds, the demand rates and beta are inputs, and every one of them can be argued with: the report is only as good as the least defensible of them, and **that is where a reviewer will go**. It uses the simplified scales of lesson 19-01, and a real machine is assessed against the standards that apply to it (for mobile robots and for machinery, with a safety integrity level for each function), by a person qualified to sign, and very likely by a third party. And "defensible" means the argument holds together, not that nothing can go wrong: three catastrophes remain at **medium**, and **a person has put their name to that**. The decision of whether to accept it is not a calculation, and no code can make it.

## What Breaks First

- **A mitigation that is a promise.** See `E-SAFE-0040`.
- **A mitigation that was true before the last change.** See `E-SAFE-0041`.
- **Credit for redundancy that is not independent.** See `E-SAFE-0042`.

## Ship It

`rc/safety/assessment.hpp` holds `Mitigation`, `Entry`, `Row`, `Finding`, `Report`, `combined_pfd` and `assess`.

This ends phase 19: a stop path that works without the application, outputs that fail to a stop, limits from range, fence, power and tilt, a supervisor that cannot be made less strict by a fault, a self-test, a black box, the arithmetic of residual risk, and an assessment that checks itself.
