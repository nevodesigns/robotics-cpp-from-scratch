# What Angles Put It There: Two Answers, or None

> Forward kinematics has one answer. This direction has a set, and half of
> robotics is about which member of it you meant.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 13-01, 03-02

## The Problem

Lesson 13-01 answered where the tool is, given the angles. Every useful thing an
arm does needs the other direction: here is where I want the gripper, what
should the motors do?

That looks like the same problem run backwards and it is not. It is harder in
three specific ways, and all three are about the **answer** rather than about
the arithmetic:

- There is usually more than one.
- Sometimes there is none.
- At the boundary between those two, the arithmetic produces `nan` more often
  than not.

## The Concept

### Two answers, and the maths prefers neither

Draw a two link arm reaching a point. Now flip the elbow to the other side. The
tool is in exactly the same place.

Both are correct. `acos` answers between zero and pi, so it hands back one of
them and the other is its negation:

```cpp
const double q2 = std::acos(cos_q2);   // one of two, and which one is arbitrary
```

Taking that as *the* answer means the choice of posture is being made by the
sign convention of a library function, which knows nothing about the table, the
base of the robot, or where the operator is standing.

The arm still reaches every target. It just occasionally swings through
something on the way, and nothing in the code chose that.

This is not a quirk of two link arms. **An inverse problem returns a set.** A
six axis arm has up to eight configurations for a pose. A redundant arm has
infinitely many. The number changes; the obligation does not, which is to return
what you found and let the caller pick on grounds the maths cannot know:

- which is nearest the arm's current pose, so it moves least,
- which keeps the elbow away from the obstacle,
- which is inside the joint limits, since a valid answer can ask for an angle
  the joint does not have.

### Two boundaries, and the near one is easy to forget

**Too far** is obvious: beyond the sum of the links there is nothing.

**Too close** is not. Inside the difference between the links there is a hole
the arm cannot fold into. A 0.5 and 0.3 arm cannot reach anything nearer than
0.2 metres to its own shoulder, and targets near the base look perfectly
innocuous.

Outside either boundary the law of cosines gives an argument outside minus one
to one, and `acos` of that is `nan`, which then travels into every joint command
that follows.

### The boundary is not a corner case

Here is the measurement that decides how this function is written.

Take the arm's own forward kinematics, fully extend it, and ask the solver to
reach the point it just reported:

```text
argument = 1.00000000000000066613       (that is 1 + 6.7e-16)
acos     = nan
```

The target is reachable. The arm is standing on it. And the arithmetic says no.

Across a hundred thousand fully extended configurations, **52,462 produced an
argument above one**. More than half. Reaching the edge of the workspace is an
ordinary thing for an arm to do, so this is the common case there, not an
unlucky one.

Which is why the clamp is not defensive noise:

```cpp
if (cos_q2 > 1.0) cos_q2 = 1.0;
if (cos_q2 < -1.0) cos_q2 = -1.0;
```

And why the reachability check needs a **tolerance**. Refusing a point the arm
can very nearly touch is its own kind of wrong. The tolerance and the clamp do
different jobs: one stops a reachable point being refused, the other stops an
accepted point producing `nan`.

### atan2, never a division

```cpp
q1 = std::atan2(y, x) - std::atan2(l2 * sin(q2), l1 + l2 * cos(q2));
```

Both of those are `atan2` and neither is a ratio. `atan2` knows which quadrant
the answer is in; a division does not, so a target behind the arm gets solved as
though it were in front. Lesson 06-01 made the same argument about `acos`, and
this is the same family.

### When a round trip is worth something

Lesson 05-03 argued that round tripping through your own code proves only that
two halves agree.

Here the round trip is fair, and the reason is specific: lesson 13-01 checked
the forward kinematics against a **closed form derived without it**, at 3721
configurations. That half has an external check behind it, so driving a solution
through it is evidence rather than a tautology.

Measured: 992 solutions driven through the arm, worst miss 3.14e-16 metres.

The general rule is worth keeping. A round trip is evidence exactly when one of
its halves was verified some other way.

### The thing that is not a bug

Near the boundary the answer changes very fast. Measured, a target moving
0.0099 metres moves the elbow 0.2944 radians, about thirty times more.

The solver is correct at both points. That is a **singularity**: the arm loses
the ability to move in some direction, and the joint speeds needed to follow a
straight line through it go to infinity. It is a planning problem, not a solver
problem, and recognising it matters mostly so that it is not treated as a
defect.

It is also exactly where the two solutions have merged, which is why the solver
reports that.

## Build It

Implement `solve` and `with_elbow` in `exercise/solution.hpp`. Return both
answers, or the reason there are none.

```
rcpp verify 13-02
```

The suite checks the refusals, that the two answers are genuinely different,
that they merge where they should, that nothing is ever `nan` anywhere in the
workspace, and then drives every solution back through the arm from 13-01.

## Use It

Closed forms exist for the arms that were designed to have them, and a great
many industrial arms were, on purpose, because a closed form is fast and exact
and enumerates its solutions.

When there is no closed form, the answer is numerical: start somewhere and
improve. That is the next lesson, and everything in this one still applies to
it, because the answer is still a set and the boundaries are still there.

## What Breaks First

- **One answer returned where there are two.** The posture gets chosen by a
  sign convention. See `E-KIN-0001`.
- **A target outside the workspace, answered anyway.** See `E-KIN-0002`.
- **`acos` of something a fraction above one.** See `E-NUM-0012`.

## Ship It

`solve` joins `rc::kin` beside the chain. Between them the arm can be asked
both questions, and the harder one answers honestly: here is the set, here is
when it is empty, and here is when its members have merged.
