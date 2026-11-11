# What Can Go Wrong, Written Down: A Hazard Log, a Risk Matrix and the Rule That a Catastrophe Is Never Low

> A hazard you have not written down is a hazard you have not decided about. Phase 19 is about the machine that has to be safe, and it starts with the list.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 18-12, 03-03, 01-06, 01-05

## The Problem

Eighteen phases built a robot that works. Nothing so far asked what happens when it works **wrongly**, near a person, with a battery that can burn and motors that can break a hand. Safety is not a feature added at the end. It is a list kept from the start, and each item on the list is answered by something that exists and is tested.

The usual failure is not a missing safeguard. It is that nobody wrote down what the safeguards were for. A team that says "we have an emergency stop" cannot say which hazards it covers, which it does not, or who is answerable for the ones left over. This lesson is the record that makes those questions answerable: a **hazard log**, a **risk class** computed the same way every time, and a query that finds what has been left with nothing done about it.

## The Concept

### A hazard is a sentence with two numbers and an owner

A hazard here is: an id; a description ("runs into a person"); a **severity** from 1 to 4 (1 negligible, 2 minor injury or damage, 3 serious injury or major damage, 4 death or loss of the machine); a **likelihood** over the life of the machine from 1 to 5 (improbable, remote, occasional, probable, frequent); an **owner**, one named person who answers for it; and a list of **mitigations**, what is done about it.

The log refuses an entry that is not well formed, and answers with the **first** thing wrong, in a fixed order: no id, an id already used, a severity outside 1 to 4, a likelihood outside 1 to 5, no owner. Seven entries were tried, six of them wrong in one way or another (an empty id, a repeated one, severity 5, likelihood 0, no owner, and one wrong in everything, which was refused as having no id), and the log held **1** afterwards. A refused hazard is not kept half way. A log with a blank owner is a log in which everything is somebody else's. See `E-SAFE-0002`.

### The score, and the line that is not a product

The score is severity times likelihood, 1 to 20, and 0 for either out of range. The class: up to 4 is **low**, 5 to 9 **medium**, 10 to 14 **high**, 15 or more **unacceptable**. Measured across the matrix:

| | improbable | remote | occasional | probable | frequent |
|---|---|---|---|---|---|
| severity 4 | medium (4) | medium (8) | high (12) | unacceptable (16) | unacceptable (20) |
| severity 3 | low (3) | medium (6) | medium (9) | high (12) | unacceptable (15) |
| severity 2 | low (2) | low (4) | medium (6) | medium (8) | high (10) |
| severity 1 | low (1) | low (2) | low (3) | low (4) | medium (5) |

Look at the top left. Severity 4 with likelihood 1 scores **4**, and by the product alone that is **low**: the same as a scuffed bumper that happens twice. It is not. The likelihood is an estimate, made by the people who built the machine and want it to be safe, and it is the number most easily moved by hope. The severity is what the outcome **is**. So there is one rule that is not arithmetic: a severity of 4 is **never classed below medium**, however unlikely it is thought to be. The cell reads `medium (4)`: the score stays honest, the class does not. Severity 3 with likelihood 1 is low, because the floor is for catastrophe only. See `E-SAFE-0001`.

### Ranking, and the list of what is open

`ranked()` sorts the log worst first by score, then by severity, then by id. The tie breaks matter because two hazards at 12 are not equal: one at severity 4 and likelihood 3 comes before one at severity 3 and likelihood 4, and the id makes the order the same on every machine. Five hazards scored 20, 12, 12, 10 and 2 came out as `H-top, H-a, H-c, H-b, H-low`.

`unaddressed()` answers the question that matters: of the hazards that are not low, which have **no mitigation**? Six hazards went in: a runaway on loss of link (unacceptable), a collision with a person (high), a very unlikely fire (severity 4, likelihood 1, so medium by the floor), a rollover with a tilt limit (medium, handled), a scuffed bumper and a minor pinch (low, nothing done, and that is fine). The answer was **U4, U1, U5**, worst first: the three that are not low and have nothing. The very unlikely fire is on the list. By the product it would not have been. See `E-SAFE-0003`.

### Eight hazards for a ground vehicle

An autonomous ground vehicle's first list, scored with the code above (`rc::sim` and `rc::nav` have built most of the machine this list is about):

| hazard | score | class | what |
|---|---|---|---|
| G2 | 16 | unacceptable | keeps driving when the link is lost |
| G1 | 12 | high | runs into a person |
| G4 | 12 | high | cannot stop in the distance its sensor sees |
| G8 | 12 | high | a sensor fails and nothing notices |
| G3 | 9 | medium | rolls over on a slope |
| G7 | 9 | medium | leaves the area it is allowed in |
| G5 | 8 | medium | battery fails in the field |
| G6 | 6 | medium | motor overheats and burns |

**1 unacceptable, 3 high, 4 medium, and with nothing done yet, 8 open.** Every lesson in the rest of this phase is an answer to a row of this table, and the last one closes the table. The first answer is the unacceptable one: G2, a machine that keeps driving when nothing is in control of it, is the reason the next lesson in this phase exists.

## Build It

`risk_score`, `risk_class`, `HazardLog::add`, `ranked` and `unaddressed` in `exercise/solution.hpp`. The hazard type and the classes are given.

```
rcpp verify 19-01
```

## Use It

`rc/safety/hazards.hpp` opens the new module `rc::safety`.

```cpp
rc::safety::HazardLog log;
rc::safety::Hazard h;
h.id = "G2"; h.description = "keeps driving when the link is lost";
h.severity = 4; h.likelihood = 4; h.owner = "controls lead";
if (log.add(h) != rc::safety::AddResult::ok) return 1;
for (const std::string& id : log.unaddressed()) std::cout << id << " has nothing done about it\n";
```

**What this does not do.** A log is only as honest as the people filling it. It does not find hazards: that takes someone who has watched the machine fail, and the methods that help (a walk through the machine's states, a list of what each part does when it breaks) are the next lessons. The numbers are a convention: the scale of 1 to 4 and 1 to 5 here is a simplification of the scales in the standards for machinery and for mobile robots, and **a real assessment uses the scale of the standard that applies to the machine and is signed by a person who is qualified to sign it**. This module is the shape of the record, not a certificate. It also says nothing about **how much** a mitigation reduces a risk: that comes later in this phase.

## What Breaks First

- **Classifying a catastrophe by the product.** A severity of 4 and a likelihood of 1 scores 4 and reads low. See `E-SAFE-0001`.
- **A hazard with no owner, or half in the log.** Everything is somebody else's. See `E-SAFE-0002`.
- **A list nobody can ask what is open.** The emergency stop is "for safety", and nobody can say which hazards it covers. See `E-SAFE-0003`.

## Ship It

`rc/safety/hazards.hpp` holds `RiskClass`, `describe`, `risk_score`, `risk_class`, `Hazard`, `AddResult` and `HazardLog`.
