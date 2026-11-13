# Feeling the Ground: Contact From the Torque at the Joints, With a Blank After Lift-off and a Gap Between On and Off

> A foot with no sensor still knows when it has touched, if it listens to its knee, and it should not believe the first noise.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-06, 34-02, 31-04

## The Problem

34-04's timing says **when a foot should be on the ground**. The **real ground** does not keep to the timetable: a foot comes down **early** on a bump, **late** over a hole, **slides** on a slippery floor. A walking robot has to **feel** whether a foot is down, and most small quadrupeds have **no force sensor in the foot**. They have the **torque at the joints** (from the motor current), and 34-02's inverse of the transpose turns it into the **force at the foot**. This lesson is the **decision** that follows: from a noisy force, **is the foot on the ground?**

## The Concept

### An estimate of force

The vertical force at the foot, from the three joint torques, is the **estimate** this lesson works with: it is noisy (the current is, and the model of the leg is not exact) and it has **artifacts**: the leg's own **acceleration** when it lifts gives a spike that is not the ground. Here a foot carried **29.4 N** in the stance with **1 N** of noise, read at **500 Hz**.

### On, off, and a run of readings

Contact is declared by **three guards**:

1. **A threshold to go on and a lower one to go off.** With one threshold, a force **hovering near it** (a foot grazing the ground) makes the state **flap**: with the force at 8 N plus or minus 2 and one threshold at 8, the state changed **10** times in 0.4 s; with **8 N on and 4 N off** it changed **0**. See `E-LEG-0018`.
2. **A run of readings in a row** (here **3**) before a change is believed, so that **one noisy reading** does nothing. The price is a **delay** of that many readings: touchdown was found **12 ms** after the force began to rise, and lift-off **2 ms** after the lift was commanded (the force had been falling for 20 ms before that, so it was found as soon as it had fallen far enough).
3. **A blank after the foot is told to lift**, during which a touchdown is **not believed**. The leg's own acceleration gave a **spike** of 12 N for 30 ms just after the lift was commanded: **without the blank** the state made **24** changes in six steps, **with it** **12**, which is the right number (**12**: one touchdown and one lift-off a step). See `E-LEG-0019`.

### What the answer is used for

The state says whether the **foot is down**. The gait (34-04) says whether it **should be**. When they **disagree** the robot has found something: a foot down **early** means a **bump** (hold the swing and take the load), one **late** means a **hole** (keep reaching); a foot that is down and moving (the stance velocity of 34-04 is not zero) means **slipping**. Those decisions (35-08, 36-03) start here. See `E-LEG-0020`.

## Build It

`contact_update` in `exercise/solution.hpp`.

```
rcpp verify 34-07
```

## Use It

`rc/legged/foot_contact.hpp`.

```cpp
rc::legged::ContactState state;  rc::legged::ContactParams p;
const bool down = rc::legged::contact_update(state, estimated_force, time_since_lift, p);
```

**What this does not do.** The force **comes from the model of 34-02**, and a **wrong zero** (34-08) or a **joint with friction** (29-04) puts an error in it that is **not noise**. The thresholds are **fixed numbers**: a heavier robot or a softer floor needs others. Only the **vertical force** is used, and a foot on a **slope** or **sliding** is not told apart from one on level ground. **Slip** and **early or late touchdown** are **not decided** here, only whether the foot is down. The **spike** is a **simulated** one of fixed size and timing; a real leg's depends on its mass and its acceleration.

## What Breaks First

- **One threshold for on and off.** See `E-LEG-0018`.
- **A touchdown believed during the lift-off.** See `E-LEG-0019`.
- **A contact sense taken as the gait's timetable.** See `E-LEG-0020`.

## Ship It

`rc/legged/foot_contact.hpp` holds `ContactParams`, `ContactState` and `contact_update`.
