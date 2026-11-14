# Putting a Peg in a Hole: A Chamfer, a Friction Cone, and a Hand That Gives Sideways So the Peg Can Find the Hole

> Insertion works when the hand is stiff in the direction it pushes and soft in the directions it should let the part choose.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-04, 29-06, 31-03

## The Problem

31-03 finds the part to a few millimetres, and a peg going into a hole is made to a tenth of a millimetre. The arm's position error is larger than the clearance, and no amount of accuracy in the camera or the arm will close that gap cheaply. What closes it is the hand giving way sideways so that the parts themselves find each other. This is the insertion of 29-06's compliance, put to work.

## The Concept

### A hole with a lip

A peg goes into a hole with a clearance (the hole's diameter less the peg's, here 100 micrometres) and a chamfer (a slope around the lip, 3 mm wide at 45 degrees). If the peg is within half the clearance of the axis it goes in. If it is beyond the chamfer's reach it misses and lands on the flat. In between it lands on the slope.

### What the slope does

Pushing down on a slope gives a sideways force: the push times (tan of the slope's angle less the friction coefficient). That force is what moves the peg towards the hole, if the hand lets it. A hand with a sideways stiffness k moves by that force over k. So the push needed to bring a peg offset by e in over the lip is k times (e less half the clearance) over (tan angle less mu). See `E-ARM-0088`.

### Soft, medium and stiff

A peg 2 mm off needed 0.8 N from a hand of 300 N/m sideways, 5.6 N from one of 2000 N/m, and 279 N from a stiff one of 100000 N/m, more than the 50 N the hand could give: it jammed. Over offsets from 0 to 4 mm the soft hand got the peg in 13 times of 17, the stiff one 2: the stiff one succeeded only for offsets of a fraction of a millimetre (the clearance and a little more), which is more accurate than an arm and a camera can be counted on to put it. See `E-ARM-0089`.

### A slope the peg cannot slide down

If the slope is not steeper than the friction cone (tan of the angle at most mu), no push moves the peg sideways; it just presses harder. At a friction of 1.2 the peg could not slide (0, 0 for no) however soft the hand. A bigger chamfer angle, or less friction (a little oil), is what fixes it, not more force. See `E-ARM-0090`.

## Build It

`push_to_centre` in `exercise/solution.hpp`.

```
rcpp verify 31-05
```

## Use It

`rc/arm/insertion.hpp`.

```cpp
rc::arm::PegHole hole;
if (rc::arm::insert(hole, offset, hand_k, 50.0) != rc::arm::kInserted) back_off_and_retry();
```

**What this does not do.** It is quasi static and in one sideways direction: a real insertion has two, a tilt of the peg, and a wedge (the peg touching at two points) that is the classic way a tight fit jams; none of that is here. The compliance is a spring at the hand, which on a real arm is made by a control law (29-06) or by a remote centre compliance device. The friction is a single coefficient. The search patterns that real cells use (spiral, wiggle) are not here, and a failed insertion is for 31-02's retry.

## What Breaks First

- **A stiff hand and an offset larger than the clearance.** See `E-ARM-0088`.
- **Pushing harder when the peg is stuck.** See `E-ARM-0090`.
- **A hand so soft the peg is missed.** See `E-ARM-0089`.

## Ship It

`rc/arm/insertion.hpp` holds `PegHole`, `InsertResult`, `can_slide`, `push_to_centre` and `insert`.
