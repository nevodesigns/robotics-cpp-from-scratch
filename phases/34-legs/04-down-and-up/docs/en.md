# Down and Up: The Phase of a Leg, How Long It Stands and Swings, and Where the Foot Should Come Down

> A step is a leg's turn on the ground and its turn in the air, and the foot should land where the body will be balanced over it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-03, 34-02, 26-07

## The Problem

A walking robot's feet take turns. Each leg spends part of every cycle on the ground (stance), pushing the body along, and the rest in the air (swing), getting ready to land again. Two things have to be decided: when each leg does which, and where the swinging foot lands. Put the foot down in the wrong place and the body is not balanced over it, and it falls or stumbles.

## The Concept

### Phase and duty

A gait has a period (here 0.5 s) and a duty factor: the fraction of the period that a foot is on the ground. Each leg has a phase offset, and its phase at a time is the fraction of the period it has done, from 0 up to 1. While the phase is below the duty the leg is in stance; after it, in swing. See `E-LEG-0009`.

### How many feet are down

A walk (duty 0.75, the legs a quarter of a cycle apart) always had at least 3 feet down: it is statically stable, since a body on three feet can stand still. A trot (duty 0.5, the diagonal legs together) had 2: always two feet, which cannot hold a body still, so the trot depends on keeping moving and balancing dynamically (35-02). With a duty of 0.35 there was a flight phase: 30 percent of the period had no foot down. That is a run or a bound, and it is not what these lessons build. See `E-LEG-0010`.

### What a stance foot does

A foot on the ground does not move; the body does. So in the body's frame the foot goes backward at the body's speed (and round at the body's turn): the velocity is minus the body's velocity and minus the turn about the foot's place. That is what the legs must do to move the body along without slipping, and a foot that does not is slipping (34-07).

### Where to put the foot down

Raibert's rule (from the hopping robots of the 1980s) puts the foot ahead of its default place by half the distance the body will go in the stance: then it leaves as far behind the default as it arrived ahead, and the body stays centred over the leg's travel. At 0.4 m/s with a stance of 0.25 s the foot was put down 0.050 m ahead of the hip's place and, a stance later, left -0.050 m behind it. A second term, k times how much faster the body is going than commanded, puts the foot further forward when the body is too fast (20 mm for 0.2 m/s at a gain of 0.1), and so slows it: a small speed controller in the foot placement. See `E-LEG-0011`.

## Build It

`touchdown_xy` in `exercise/solution.hpp`.

```
rcpp verify 34-04
```

## Use It

`rc/legged/stance_swing.hpp`.

```cpp
rc::legged::GaitTiming gait;  const double phase = rc::legged::leg_phase(gait, leg, t);
double td[2];  rc::legged::touchdown_xy(hip, v, v_cmd, t_stance, 0.1, td);
```

**What this does not do.** The timing is fixed (a period and a duty): a real gait changes both with speed and lifts a foot when it senses the ground early or late (34-07). The rule puts the foot in a plane, with no thought of where the ground is (a step or a hole, 35-08) or whether it is in reach of the leg (34-01). The body's velocity is known exactly. Turning is only in the foot's velocity, not in the touchdown. The numbers are typical.

## What Breaks First

- **A phase that does not wrap.** See `E-LEG-0009`.
- **A trot taken for a gait that can stand still.** See `E-LEG-0010`.
- **A touchdown under the hip at speed.** See `E-LEG-0011`.

## Ship It

`rc/legged/stance_swing.hpp` holds `GaitTiming`, `leg_phase`, `in_stance`, `count_stance`, `stance_foot_velocity` and `touchdown_xy`.
