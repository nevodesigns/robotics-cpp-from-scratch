# What the Motors and the Battery Allow: Top Speed, Acceleration and Climb From a Drive Train, and Which of the Motors or the Grip Is the One That Runs Out First

> A vehicle has two kinds of limit, what its motors can push and what its tyres can hold, and the lower of them is the only one that counts.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-04, 23-05, 22-05, 21-10

## The Problem

The vehicle of lesson 23-05 can be asked to do whatever its grip allows. It is also being pushed by motors, which have their own limits, and those limits are not the grip's. A small robot with strong tyres on tarmac is limited by its motors; the same robot on ice is limited by its grip, and the best motors in the world do not change that. A planner that knows only one limit asks for accelerations the other will not give. This lesson finds the motors' limits and puts them beside the grip's.

## The Concept

### The force at the wheel

A brushed motor gives its stall torque at rest and none at its no load speed, and in between a straight line (lesson 22-04). `motor_force` takes a speed of the vehicle and a voltage on the bus and gives the force at the wheels of **all the motors together**: the motor's torque from its current (limited by the driver) less the viscous drag, times the gear ratio and the efficiency of the gearbox, over the wheel radius. With the battery's internal resistance set to nothing, the force at rest was the stall force, and at half the no load speed it was half of it, and at the no load speed of **1.83 m/s** it was nothing.

### The battery sags

A battery has an internal resistance, and a standing start draws the most current. The voltage the motors see is the battery's less the resistance times the current, and the current depends on the voltage, so `bus_voltage` finds where they settle by repeating. A battery of 0.5 ohm took the bus below three quarters of its voltage at a standing start, and **cut the acceleration from 4.54 to 3.32 m/s2** compared with one of 0.02 ohm. It recovers as the vehicle speeds up and the motors draw less. A vehicle that was tuned on a fresh battery will not accelerate like that on a tired one. See `E-VEH-0016`.

### Top speed and climb

The top speed is where the force of the motors is equal to what pushes back: the rolling resistance, the pull of the slope along the heading, and the drag of the air. On level gravel the reference vehicle's top speed was **1.67 m/s**, a little under the no load speed; on a slope of 10 degrees, **0.99 m/s**. A slope the motors cannot start on gives a top speed of zero. The steepest slope they can start on is where the standstill force is all spent against gravity and the rolling resistance: **25.6 degrees**. See `E-VEH-0017`.

### Which limit is the limit

The grip of gravel holds a slope of **22.3 degrees** (lesson 22-05) and the motors can start on one of **25.6**: on gravel the grip runs out first. On tarmac the grip holds **38.1 degrees**, more than the motors can: there the motors run out first. `achievable_accel` takes the lower of the acceleration the motors give and the grip allows at a standstill and says which it was. On tarmac it was the **motors**, on gravel the **grip** and on ice the **grip**. Buying stronger motors for a robot that is grip limited on its floor is money spent on nothing; buying better tyres for one that is motor limited is the same. See `E-VEH-0018`.

## Build It

`motor_force` in `exercise/solution.hpp`.

```
rcpp verify 23-06
```

## Use It

`rc/vehicle/drive_train.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::DriveTrain d;
const rc::vehicle::Achievable a = rc::vehicle::achievable_accel(d, terrain.at(x, y), heading, speed);
planner.set_acceleration_limit(a.accel);
```

**What this does not do.** The motors are all the same and share the load evenly, and the gearbox has one efficiency at all loads and speeds, where a real one is worse at light load. There is no thermal limit: a motor that can give its stall torque for a second cannot give it for a minute, and the driver's current limit is a constant. The inertia of the motors and wheels is not counted in the acceleration, which for a small robot with a heavy gearbox matters. And the battery's resistance is one number, where a real battery's depends on its charge, its temperature and its age.

## What Breaks First

- **Motors tuned on a fresh battery.** See `E-VEH-0016`.
- **A top speed that ignores the slope.** See `E-VEH-0017`.
- **The wrong limit upgraded.** See `E-VEH-0018`.

## Ship It

`rc/vehicle/drive_train.hpp` holds `DriveTrain`, `MotorConstants`, `motor_constants`, `motor_current`, `motor_force`, `bus_voltage`, `drive_force`, `resistance`, `top_speed`, `max_accel_motors`, `max_climb`, `Limit`, `Achievable` and `achievable_accel`.
