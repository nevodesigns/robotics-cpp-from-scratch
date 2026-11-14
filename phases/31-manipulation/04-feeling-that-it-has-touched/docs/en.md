# Feeling That It Has Touched: A Torque Residual, a Threshold Above the Model's Error, and a Count of Ticks Before Believing It

> A robot with no force sensor still feels contact, as the difference between the torque it used and the torque it should have needed.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-03, 29-06, 29-02

## The Problem

31-02 decided a grasp had worked from the gap between the fingers. Much of manipulation is knowing that the tool has touched something (the table, the part, the wall of a box) with no force sensor at all, which is what most arms have. The joints' motors give their current, and from the current a torque. A free arm needs a torque the model can say (gravity, friction, inertia, 29-01 and 29-04). When the measured torque is different from that, something is pushing on the arm.

## The Concept

### The residual

The residual is the measured torque less the torque the model says free motion needs. In free motion it should be zero; in practice it is the model's error (a gravity term that is a few percent out) and noise. When the tool meets something the residual jumps by the torque of the contact. A first order filter smooths it (a share alpha of the new residual), at the price of a delay.

### A threshold, and a count

Contact is declared when the filtered residual has been over a threshold for a number of ticks in a row. Both matter. A threshold below what the model gets wrong gives false alarms in free motion: with gravity 5 percent out, a threshold of 0.05 N m gave 1 (declared before the wall had even been reached) and one of 0.8 N m gave 0. A threshold too high feels only hard contact. The count keeps one noisy tick from being believed. See `E-ARM-0085` and `E-ARM-0086`.

### How soon, and how hard

A joint driven at 0.2 rad/s into a stiff wall was found to have touched 15 ms after it reached it, with the wall pushing back 1.31 N m by then. At 0.6 rad/s the detection took 10 ms, and by the time the arm had stopped the wall's torque had reached 5.20 N m: a faster approach is a harder hit before the detection can do anything about it. The speed the arm may use when it expects contact is set by this. See `E-ARM-0087`.

## Build It

`contact_update` in `exercise/solution.hpp`.

```
rcpp verify 31-04
```

## Use It

`rc/arm/contact.hpp`.

```cpp
rc::arm::ContactDetector det;  det.thr = 0.5;
if (rc::arm::contact_update(det, tau_from_current, tau_from_model)) stop_and_hold();
```

**What this does not do.** It looks at one joint at a time; the force at the tip comes from all the joints through the Jacobian (28-03), and a contact at a link is not the tool's. The model's torque is taken from the simulation, with the acceleration known exactly; a real one has the acceleration from differencing an encoder, which is noisy, and friction that is not a number (29-04). The motor current is taken as the torque, which is not true through a gear with friction. The wall is stiff and fixed; a soft thing gives a slower rise and a later detection.

## What Breaks First

- **A threshold below the model's error.** See `E-ARM-0085`.
- **A count of one tick.** See `E-ARM-0086`.
- **Driving at speed into something the arm is to feel.** See `E-ARM-0087`.

## Ship It

`rc/arm/contact.hpp` holds `ContactDetector` and `contact_update`.
