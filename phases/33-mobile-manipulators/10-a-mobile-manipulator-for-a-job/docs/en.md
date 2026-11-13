# A Mobile Manipulator for a Job: Every Check of the Phase Run Against One Job, and the First One That Fails Named

> A job is released when the base can park for it, the arm can carry it, the grasp is likely, the battery lasts, and the people around it are safe, and not before.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-09, 33-08, 33-07, 33-06, 33-03, 27-08

## The Problem

Nine lessons have each answered one question about a base with an arm on it: where the frames are, where the arm reaches, where to park, how the two move together, when it tips, how much it carries, how well it must know where it is, what the mission is and how fast it may go among people. A **job** needs **all** of the answers to be yes, **at once**, and the first thing a customer or a safety officer asks is: **can it do this, and if not, which part says no?** This capstone runs every check of the phase against one job and **names the first that fails**, in the manner of 27-08's release for a vehicle and 32-06's choice for an arm.

## The Concept

### A job and a robot, as numbers

A **job**: where to pick and where to put (x, y, z in the world), the payload, the length of the route and the speed wanted on it, how close people come to the route, how well the base stops, and the obstacles. A **robot**: the arm on its base and its mount (33-01), the tucked pose for carrying and the reaching pose, the masses, the arm's model and joint limits, the drive, the errors of the localisation, the camera's error at working range and the person model of 31-06. Writing them down is most of the work, as in 32-06.

### Seven requirements, in order

1. **Parking** (33-02, 33-03): there is a place for the base to stop from which the arm reaches the pick, and one for the place, clear of the obstacles. A shelf ringed by obstacles gave **none**, and the job was refused at requirement **1**.
2. **Stopping** (33-02): the tolerance of the ring (**35 cm**) must be **at least three times** the sigma of the base's stopping (**3 cm**). A base that stops to **15 cm** failed at requirement **2**.
3. **Payload** (33-06): the least of the arm's, the tipping and the traction limits (**4.3 kg**) must be at least the job's. A job of **8 kg** failed at **3**.
4. **Grasp** (33-07): with the camera on the hand, the chance of a good grasp must be at least **0.99** (it was **1.000**). A camera good to only **12 mm** failed at **4**.
5. **Battery** (33-06): the range with the load (**13.1 km**) must be at least **1.2 times the route**. A route of **30000 m** failed at **5**.
6. **Travel speed** (33-05, 33-09): the speed from which the loaded base can still stop in its distance without tipping must be at least the speed wanted. A fast route with a load failed at **6**.
7. **People** (31-06): the speed that the nearest person allows must be at least a **useful minimum**. A person **0.45 m** from the route failed at **7**.

A job of **2 kg** over **6 m** passed all seven (released: **1**, 1 for yes). The **first failure** is what is reported, and the others are still computed and kept, so the figures show **how far** each is from passing. See `E-MOB-0028`.

### Why the order

The requirements are ordered by **what can be changed cheapest last**: a place to park is a matter of the floor plan; the stopping accuracy, the payload and the camera are the **robot's design**; the battery, the travel speed and the people are the **job's settings**. When the first to fail is a design one, no change of setting will help, and saying **which** is the point. See `E-MOB-0029` and `E-MOB-0030`.

## Build It

`check_job` in `exercise/solution.hpp`.

```
rcpp verify 33-10
```

## Use It

`rc/mobile/mm_job.hpp`.

```cpp
const rc::mobile::JobVerdict v = rc::mobile::check_job(robot, job);
if (!v.released) report_the_first_failure(v.first_fail);
```

**What this does not do.** Each check is the **simple model of its lesson** with the **typical numbers** (the lessons say which): it is a **screen** that finds jobs that cannot work, not a proof that a job that passes will. The **mission machine** of 33-08 is not run here, nor the **whole-body motion** of 33-04 (the arm's motion between stowing and reaching); the **route** is a length and a speed, not a path, and the **people** are one separation. **Fault handling** (a grasp that is retried, a part that drops) is the mission's. A real release needs the **trial on the real robot**, with its reaction time and its stopping measured (27-08's list for a vehicle is the shape of it).

## What Breaks First

- **A job released on the arm's data sheet alone.** See `E-MOB-0028`.
- **The first failure hidden by a long list.** See `E-MOB-0029`.
- **A design limit that was reported as a setting.** See `E-MOB-0030`.

## Ship It

`rc/mobile/mm_job.hpp` holds `Job`, `Robot`, `JobVerdict` and `check_job`. With it phase 33 is complete: a wheeled base with an arm that fetches a thing from one place and carries it to another, with the frames of both together, where to park, how the two move as one, what the arm's reach and load do to the balance and the speed, how well the base must know where it is, the whole job as a mission under an interlock and a speed supervisor, and a check that says whether a job can be done and which part says no.
