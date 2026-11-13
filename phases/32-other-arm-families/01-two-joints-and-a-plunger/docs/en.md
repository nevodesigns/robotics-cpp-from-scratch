# Two Joints and a Plunger: The SCARA, Its Closed Form Reach, Its Two Elbows, and Why Its Joints Carry No Weight

> An arm built for the flat table gives up the third dimension in the joints and gets stiffness and speed in exchange.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-04, 28-06, 29-01, 31-07

## The Problem

The arms of phases 28 to 31 are **six axis** arms: any place, any direction. A great many tasks are **flat**: parts on a table or a conveyor, picked from above and put down above. For those the sixth, fifth and fourth axes are wasted, and what the job wants is **speed**, **stiffness** and a **price**. The family built for that is the **SCARA** (selective compliance articulated robot arm): two joints that turn about **vertical** axes, a **vertical slide**, and a **roll** at the tool.

## The Concept

### Forward kinematics, flat

With link lengths **l1** and **l2** (here **0.225** and **0.225 m**, typical of a 450 mm reach), the tool is at (l1 cos q1 + l2 cos (q1 + q2), l1 sin q1 + l2 sin (q1 + q2)), at a height that is the **slide**'s alone, and its heading is q1 + q2 plus the roll. Each of the four values has **one job**, which is why the arm is easy to control and fast.

### Two elbows, in closed form

Reaching (x, y) is the two link problem: cos q2 is (x squared + y squared less l1 squared less l2 squared) over 2 l1 l2, and q2 is **plus or minus** its arccosine: **two elbows**, left and right. Then q1 is the direction of the point less the angle that the second link makes. The grid test reached **159** of **272** (a point and an elbow) and the tool was at the asked place to **0.000000000 m**. A point **0.5 m** out was refused (**0**), and so was a point the **joint limits** forbid (an arm cannot fold back on itself). See `E-ARM-0103`.

### Where it loses a direction

The planar Jacobian's determinant is **l1 l2 sin q2**: **0.0506** at **1.57 rad** and **0.0000** when straight, and zero folded. At those poses the tool cannot move **along** the arm: the same singularity as 28-06, now in the shape that matters for a flat arm. The work area is kept away from the edge of reach. See `E-ARM-0104`.

### Why the joints carry no weight

The first two axes are **vertical**, so gravity is **along** them and puts **no torque** on them. A payload of **2 kg** put **0.0 N m** on those joints, where the shoulder of a six axis arm holding it **0.4 m** out carried **7.85 N m** (29-01). The slide carries the weight directly. This is where the SCARA's **speed** comes from (small motors, no gravity compensation, 29-02, in the first two joints) and its **stiffness** in the vertical direction; it is **compliant sideways** and stiff vertically, which is what its name says, and which suits pushing a part **down** into a hole (31-05). See `E-ARM-0105`.

## Build It

`scara_ik` in `exercise/solution.hpp`.

```
rcpp verify 32-01
```

## Use It

`rc/arm/scara.hpp`.

```cpp
rc::arm::Scara arm;
double q1 = 0.0, q2 = 0.0;
if (!rc::arm::scara_ik(arm, x, y, +1, q1, q2)) try_the_other_elbow_or_refuse();
```

**What this does not do.** The link lengths and limits are **typical**, not one maker's. The **slide and roll** are taken as independent and exact. **Dynamics** (the inertia of the second link on the first, which limits the acceleration) and **joint speed limits** are not here; 31-07 and 30-06 are how a cycle's time would be worked out. The **elbow** choice is not made for you: which is better depends on where the arm is going next and on what is in the way.

## What Breaks First

- **One elbow assumed.** See `E-ARM-0103`.
- **A task placed at the edge of reach.** See `E-ARM-0104`.
- **A SCARA given a job that needs a tilted tool.** See `E-ARM-0105`.

## Ship It

`rc/arm/scara.hpp` holds `Scara`, `ScaraTool`, `scara_fk`, `scara_ik`, `scara_det` and `gravity_torque`.
