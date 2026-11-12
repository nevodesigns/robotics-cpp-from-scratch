# A Skid Steer and Its Centre of Rotation: Why Four Fixed Wheels Turn Only by Scrubbing, What That Costs, and Whether the Motors Can Pay

> A vehicle that cannot steer its wheels turns by dragging them sideways, and the drag is a force the motors have to find.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-01, 22-04, 22-05, 21-10

## The Problem

A car turns by turning its front wheels so that every wheel rolls along its own circle. A vehicle whose wheels are fixed to the chassis has no such choice. To turn it, the left wheels are driven slower than the right, and the two sides then want to go in different directions, and the tyres, which can only roll along the chassis, are dragged sideways across the ground. That is **skid steering**, it is how most small field robots and many large machines are turned, and it works because the ground lets the tyres slide. What the ground lets them do, and what it costs in force, is the subject of this lesson.

## The Concept

### It is a differential drive, widened

Kinematically a skid steer vehicle is the differential drive of lesson 23-01 with a turn that is **less than the wheels say**, and `SkidSteer` is exactly that: a `DiffSlip` with a turn efficiency of **1 / chi**, where `chi` is the **ICR coefficient**. A coefficient of 1 is a vehicle that rolls without scrubbing, and a larger one is a track that is, in effect, wider. A skid steer of coefficient 1.4 and a differential drive with an efficiency of 1 over 1.4 followed the **same path to 1e-12**. See `E-VEH-0004`.

The **instantaneous centre of rotation** is where the vehicle is turning about at this moment, at a distance of speed over turn rate. With wheels at 20 and 30 rad/s it was **0.625 m** away at a coefficient of 1 and **0.938 m** at 1.5, exactly 1.5 times as far, because the vehicle turns 1.5 times less for the same wheels. A pivot has the centre at the middle; a straight line has it at infinity.

### The slide, and its angle

In a pivot, a wheel at a distance `x` ahead of the centre sweeps sideways at `omega * x`, and rolls along the chassis at `omega * y` with `y` its distance across. The wheel can only roll, so the sideways part is a **slide**. At the front wheels the angle of the slide is `atan(L / B)`, with `L` the wheelbase and `B` the track, and it does not depend on speed: a chassis 0.30 m long and 0.25 m wide slid its wheels at **50.2 degrees**, and a square chassis at 45. A long, narrow vehicle scrubs hard and a short, wide one lightly, which is why skid steer machines are built short.

### The force to pay for it

The scrubbing is friction, and friction is a force. The four wheels each carry a quarter of the weight and slide against it at half the diagonal from the centre, so the turning resistance is `mu * m * g * (half the diagonal)`, and the two sides, pushing in opposite directions across the track, can give `force * B`. The force each side must give to pivot is therefore

`F = mu * m * g * sqrt(L*L + B*B) / (2 * B)`.

For a 10 kg vehicle of this shape it was **34.5 N a side on gravel** and **7.7 N on ice**. Two gear motors of lesson 22-04 on a side, at the rim of a wheel of 0.05 m, can give **28.0 N**. So on ice and on grass the vehicle can pivot, and on gravel it **cannot pivot**: the motors could drive it forward on gravel all day and could not turn it on the spot. A planner that asks a skid steer robot for a turn in place is asking for a force, and the answer depends on the floor. See `E-VEH-0005`.

## Build It

`pivot_thrust_per_side` in `exercise/solution.hpp`.

```
rcpp verify 23-02
```

## Use It

`rc/vehicle/skid_steer.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::SkidGeometry g;
rc::vehicle::SkidSteer robot(g, 1.4, 0.02);
if (!rc::vehicle::can_pivot(thrust_per_side, g.mass, mu, g.wheelbase, g.track)) plan_a_wider_turn();
```

**What this does not do.** The coefficient is given, not derived. The force of the pivot assumes the weight is spread evenly over four wheels, the friction the same everywhere and the sliding complete, so it is a **least** force to start a pivot on a flat floor and says nothing about one on a slope. There is no lateral drift in the model, though a skid steer vehicle moving fast round a bend does drift. And the thrust available is the stall thrust of the motors at the rim, which is more than they give when moving. Lesson 23-08, later in this phase, finds the coefficient from a run.

## What Breaks First

- **A skid steer modelled as a car that cannot steer.** See `E-VEH-0004`.
- **A turn in place asked for on a floor that cannot pay for it.** See `E-VEH-0005`.
- **A long chassis chosen for stability.** See `E-VEH-0006`.

## Ship It

`rc/vehicle/skid_steer.hpp` holds `SkidGeometry`, `SkidSteer`, `SkidMotion`, `skid_motion`, `front_slip_angle`, `pivot_thrust_per_side` and `can_pivot`.
