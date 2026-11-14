# Motors, Gearboxes and Backlash as a Plant: A Gear Motor Built From Its Data Sheet, and the Gap That an Encoder on the Motor Cannot See

> The data sheet gives four numbers, and from them a motor can be simulated well enough to find out what a controller will do to it.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 22-03, 21-10

## The Problem

A controller is tuned on a model of the motor, and a model is only as good as the numbers it was built from. A data sheet gives four of them: the voltage, the speed with no load, the torque when stalled, and the gear ratio. From those a brushed motor can be built that has the right speed, the right torque, and a time constant that is **not** on the data sheet and decides everything about how fast a loop can be. And a gearbox has a fault that no data sheet states in a number the control engineer can use: **backlash**, the gap between the teeth, which an encoder on the motor shaft cannot see.

## The Concept

### Four numbers make a motor

A brushed DC motor is a voltage source and a resistance feeding a torque constant that is also its back voltage constant. From the supply voltage and the no load speed comes `ke = V / w_no_load`; from the voltage and the stall torque comes the resistance `R = V * ke / stall_torque_at_the_motor`. That is a complete electrical model, and the simulated motor on 12 V ran at **350.0 rpm** with no load, the data sheet's 350. Half the stall torque on the shaft gave **175.0 rpm**, half the speed, because the torque-speed line of a brushed motor is a straight line from the stall point to the no load point.

Everything in the specification that is not on a data sheet (the rotor inertia, the inertia of the wheel and the robot behind it, the viscous drag, the backlash) is marked **typical** in the code and in the header. The values stand for a 12 V, 37 mm class gear motor; they are not measurements of one. A real motor needs the same structure with measured numbers, and lesson 18-03 shows how to measure them.

### The time constant

The part a controller cares about is how fast the speed answers a voltage. It is `J * R / (ke*ke + b*R)`, where `J` is the rotor's inertia plus the load's reflected to the motor shaft (**divided by the square of the gear ratio**, which is why a gearbox makes a heavy load feel light). The formula gave **0.246 s**; a simulated step to 63 percent took **0.246 s**. The two agreeing is the check that the simulation does what the algebra says. See `E-FIELD-0010`.

### The current limit

A driver limits the current to protect itself and the motor, and that limits the torque. A limit of 1 A gave a stall torque of **0.312 N m** against **0.700 N m** with no limit that bites; the first is the number a planner must use for how steep a slope the robot can climb. See `E-FIELD-0011`.

### Backlash

A gearbox has a gap. When the direction reverses, the gear side must cross all of it before the output moves, and `GearMotor` keeps the two angles apart: `gear_angle()` is where the motor's encoder says the shaft is, and `output_angle()` is where it is. After driving forward, a reversal lost **2.02 degrees** of motion before the output moved: the whole gap.

That matters because the encoder is on the motor. A PD loop on it settles where the encoder says, exactly. The output then sits at the **edge of the gap on the side it came from**: **-1.00 degrees** from the target when it arrived from below and **1.00 degrees** from above, a **2.00 degree** difference in where the same command leaves the same robot. See `E-FIELD-0012`.

## Build It

`GearMotor::step` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Limit the voltage, find the current and limit it, find the torque, accelerate the rotor, then move the output through the backlash.

As shipped, 4 of the 5 tests fail, and these are the ones your code has to turn green:

- The data sheet numbers come back out: the no load speed, and half the speed at half the stall torque.
- The time constant from the constants is the time a step takes to reach 63 percent.
- On reversing, the gear side crosses the whole gap before the output moves.
- A position loop on the motor encoder settles where the encoder says, and the output is off by half the gap, on the side it came from.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 22-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/field/gear_motor.hpp` opens the module `rc::field`.

```cpp
rc::field::GearMotor motor;
for (int i = 0; i < 1000; ++i) motor.step(volts, load_nm, 0.001);
const double wheel_rad_per_s = motor.output_speed();
```

**What this does not do.** It has no inductance, so it cannot show current spikes, and no temperature, so the torque does not fall as the winding warms. The friction is viscous only: a real gearbox has a stiction that stops a slow motor altogether. The backlash is a clean gap with no elasticity, so there is no rattle when the teeth meet. And the load is a constant torque and an inertia; a wheel on the ground is the next lessons.

## What Breaks First

- **A controller tuned without the time constant.** See `E-FIELD-0010`.
- **A current limit forgotten in the torque budget.** See `E-FIELD-0011`.
- **An encoder on the motor trusted to say where the output is.** See `E-FIELD-0012`.

## Ship It

`rc/field/gear_motor.hpp` holds `GearSpec` and `GearMotor`.
