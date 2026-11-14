# Choosing Where to Park: Candidate Places Round the Target, a Score That Trades Reach Margin Against Travel and Clearance, and Saying None

> Many base places reach the target; the right one is reachable with room to spare, near, and clear of what is in the way.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-02, 33-01, 26-01

## The Problem

33-02 gave the region of base places from which the arm reaches a target. A planner needs one place. Many are in the region, and they are not equal: some are at the edge of the ring with no room for the base to stop short, some are far from where the base is, some are against a wall. Choosing where to park is a small search over candidate places with a score, and it has to be able to answer none.

## The Concept

### Candidates round the target

The target is the centre. The candidates are the places for the arm's axis at several distances (between the ring's radii, kept a minimum margin inside both edges) and several bearings round the target, with the base facing the target; the base's own centre follows from the mount (33-01). This is a polar grid of the ring: 252 places for 36 bearings and 7 distances. See `E-MOB-0007`.

### Reject, then score

A place is rejected if the base would be within its own radius and a clearance of an obstacle (circles on the ground). Each of the rest is scored: the margin in the ring over the tolerance (1 in the middle of the ring: the room to stop short or long, 33-02), less the travel from where the base is, plus the clearance up to a cap. Each term has a weight, and the weights are the decision: which matters more, to arrive soon or to arrive with room. See `E-MOB-0008`.

### The weights matter

With nothing in the way and the margin weighted most, the base was put at a bearing of -170 degrees from the target, 0.36 m away, in the middle of the ring (a margin of 1.00). When travel was weighted most the base stopped at the near edge and travelled 1.24 m, where with no weight on travel it travelled 2.56 m (to the best place whichever side it was on). A shorter drive with a smaller margin is a trade, and the weights say which. 

### Obstacles move the choice

With a row of obstacles across the near side the choice went round to a bearing of -30 degrees, and every place it chose kept the base's radius and the clearance from every obstacle. A planner that picks the place without the obstacles and finds the base cannot get there has wasted its time.

### Saying none

With obstacles all round the target 0 of the 252 places were free, and the answer was none. A choice that returns something anyway sends the base into a wall. The caller has to be able to ask for more room (a smaller margin, a different approach) or give up and say so. See `E-MOB-0009`.

## Build It

`choose_park` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The arm's axis is the target plus d at the bearing phi; the base faces the target (heading phi + pi), its centre from base_for_axis; drop it if its clearance to any obstacle (centre distance less the obstacle's radius and the base's) is under p.clearance; else score = w_margin * margin - w_travel * travel + w_clear * min(clearance, cap), the margin being the smaller of d - r_in and r_out - d over the tolerance; keep the best (and count the free places).

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- With nothing in the way the base is put on the near side, in the middle of the ring when margin counts most.
- Weighting travel puts the base at the near edge of the ring, weighting margin in the middle.
- Obstacles move the choice round them, and obstacles all round give none.

```
rcpp verify 33-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/mobile/parking.hpp`.

```cpp
const rc::mobile::ParkChoice c = rc::mobile::choose_park(m, ring, target_xy, base_now, obstacles, n, params);
if (!c.found) ask_for_help();
```

**What this does not do.** The obstacles are circles and the base a circle: a real base has a shape, and a heading that matters for how it will approach. The route to the place is not checked, only the place (a planner for that is phase 26, and the base's own motion model, 23). The score is a sum with fixed weights; a different task would weigh differently and the lesson does not say how to learn them. The arm's own collisions with the shelf (30) are not part of the score. Only the bearing and the distance are searched, with the base facing the target.

## What Breaks First

- **A place chosen without looking at the obstacles.** See `E-MOB-0009`.
- **A place at the edge of the ring.** See `E-MOB-0007`.
- **A choice that cannot say none.** See `E-MOB-0009`.

## Ship It

`rc/mobile/parking.hpp` holds `Obstacle`, `ParkParams`, `ParkChoice`, `base_for_axis` and `choose_park`.
