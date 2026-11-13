# How Much It Can Carry: The Arm's Torque, the Base's Braking and Traction, and Which One Says No First

> A payload has three judges, and the lightest limit among them is the payload.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-05, 29-08, 23-05, 23-06

## The Problem

"What can it carry?" has a number on the arm's data sheet, and it is **not the answer** for the mobile manipulator. The arm's rated payload is what its **joints** can hold. The same payload also has to be **carried by the base** (which has to be able to **stop** with it, 33-05, and **accelerate** with it) and has to be **carried for as long as the job takes** (the battery). The payload is the **least of the limits**, and what matters is knowing **which one says no first**, because that is the one to change if more is wanted.

## The Concept

### The wheels' limit

The base's acceleration is the **force its wheels can give over the total mass**, until the floor's **grip** gives out (the friction times g, 23-05). With 80 N of drive and 40 kg the limit is 2 m/s2; at 4 kg the force would give 20, and the grip limits it to **5.9 m/s2**. The more mass, the lower the acceleration. For a **wanted acceleration** of **0.5 m/s2** the heaviest load the wheels allow came to **130.7 kg**: the wheels were not what limited this robot. A floor with too little friction gives **no** budget at all (the test uses a friction of 0.03). See `E-MOB-0016`.

### The arm's limit

The heaviest load at a pose that every joint can hold, from the joints' torque limits and the structure's rating (29-08). With the arm reaching **0.65 m** the answer was **5.0 kg**, which is the **rating of the structure**: the joints could hold at least that.

### The tipping limit

33-05 gave the deceleration beyond which the base tips. An **emergency stop** from **1.5 m/s** in **0.20 m** needs **5.62 m/s2** (v squared over twice the distance). Find the heaviest payload for which the base can still brake that hard without tipping (by **halving an interval**: the limit falls as the load rises): **4.3 kg**. A gentler stop (0.6 m) allows more, as the test checks. See `E-MOB-0017`.

### The least, and which one

The three limits were **5.0** kg (arm), **4.3** kg (tipping) and **130.7** kg (traction), and the least was **4.3 kg**: the **binding limit was number 1** (0 the arm, 1 tipping, 2 traction). The arm is rated for more than the base can carry **if it has to stop in an emergency**, and a data sheet that says "5 kg" is **not** the number for this robot. If more is wanted, **a wider base, a lower arm, a longer stop or a slower approach** is what to change, not a stronger arm.

### What it does to the range

A payload also costs **energy**: rolling resistance grows with the mass. With **10 kg** aboard the range on the battery went from **15509 m** to **13838 m**; most of the energy here goes to the electronics (15 W idle), so the loss is modest, and a base that moves more and idles less would lose more. With no rolling resistance the mass does not matter at all (the test). See `E-MOB-0018`.

## Build It

`payload_budget` in `exercise/solution.hpp`.

```
rcpp verify 33-06
```

## Use It

`rc/mobile/payload_effect.hpp`.

```cpp
const rc::mobile::Budget bud = rc::mobile::payload_budget(m, base, q, masses, arm_model, joint_limits, rated, drive, req);
if (payload_kg > bud.best) refuse_the_job();
```

**What this does not do.** The three limits are for **one pose and one stop**; carrying means **many poses**, and the worst should be used. The **arm's own gravity** is a rod model (typical masses, 29-01). The **traction limit** is for a straight acceleration, not for a turn (23-05's circle). **Heat** in the motors (29-08) and the **battery's own limits** (23-06) are not counted. The **range** model is a rolling resistance and a constant electronics draw. The **mass of the payload** is taken as known. The numbers for the base are **typical**.

## What Breaks First

- **The arm's rating taken as the robot's.** See `E-MOB-0017`.
- **An emergency stop that was not tested with the load.** See `E-MOB-0017`.
- **A wheel force worked out for the empty base.** See `E-MOB-0016`.

## Ship It

`rc/mobile/payload_effect.hpp` holds `BaseDrive`, `Requirements`, `Budget`, `base_accel_limit`, `range_m` and `payload_budget`.
