# How Hot the Knees Get: Torque Over a Step, the Heat of the Windings, and How Heavy a Robot the Legs Can Carry

> A robot that stands on two legs at a time asks each leg for twice the torque, and heat goes as the square of it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-05, 34-02, 29-08

## The Problem

34-02 said that a crouched leg costs torque, and 29-08 said what torque costs a motor: heat. A quadruped's joints are small, hot-running motors in small, closed legs, and the knees get hot first. Heat is what limits how long a robot can stand, trot or carry a load before a motor protects itself and the legs go limp. The load on a leg is the torque over a whole step, not at a moment, and the number to watch is its root mean square.

## The Concept

### Torque in the stance

With the foot h below the hip, the knee gives the torque of 34-02 for the share of the body's weight that this leg carries: the body's weight over the number of feet down. Standing on four feet at 0.30 m, the knee of a 12 kg robot gave 4.33 N m. In a trot only two feet are down, so each carries twice as much and the knee gave 8.65 N m in the stance. See `E-LEG-0015`.

### The root mean square over the cycle

A leg is in stance for the duty (34-04) and in swing for the rest, where it gives only a small torque to move its own light mass (here 0.3 N m, typical). The heating goes as the square of the torque, so what counts is the root mean square over the cycle: the square root of (duty times the stance torque squared plus (1 less duty) times the swing torque squared). In the trot it came to 6.12 N m: far above half the stance torque, because the square weights the large torque.

### Heat, and where it settles

The heat in the windings is the current squared times the resistance, and the current is the torque over the motor's constant (29-08). The windings settle to a temperature above the air of that power times the thermal resistance: 14.0 K standing and 28.1 K in the trot, against a limit of 60 K. Heat goes as the square of the torque, so doubling the torque (two feet down, not four) quadruples the heat for the time it is on. A crouch of 0.20 m in the trot settled at 44.3 K, close to the limit. A heavy robot held in a crouch passes the limit in time: 25 s for the 40 kg robot of the test on four feet at that height. See `E-LEG-0016`.

### The heaviest body

Turning it round: by halving an interval, the heaviest body for which the knee's settling temperature stays at or under its limit in a trot at 0.30 m was 17.5 kg. A robot built for 12 kg has 17.5 kg of room in its knees at that height, in that gait, and a lot less crouched. See `E-LEG-0017`.

## Build It

`cycle_rms_torque` in `exercise/solution.hpp`.

```
rcpp verify 34-06
```

## Use It

`rc/legged/leg_load.hpp`.

```cpp
double tau[3];  rc::legged::stance_torques(leg, height, body_mass, 2, tau);
const double rms = rc::legged::cycle_rms_torque(std::fabs(tau[2]), 0.3, 0.5);
```

**What this does not do.** The stance torque is static, with no acceleration of the body and no sharing that differs from equal (the real share depends on where the centre of mass is, and impact at touchdown adds a peak). The swing torque is a number (typical). The heat model is the single lump of 29-08, with the motor constants typical of a small quadruped's joint; real ones have a controller that cuts the current before the limit, and the air flow changes with the gait. The hip's and the lean's torques are not looked at: the knee is the worst here. Only steady gait is covered: a start from cold takes time to heat (29-08's time to the limit).

## What Breaks First

- **A heat budget worked out for standing.** See `E-LEG-0015`.
- **A crouch held for a demonstration.** See `E-LEG-0016`.
- **A payload added without the knees in mind.** See `E-LEG-0017`.

## Ship It

`rc/legged/leg_load.hpp` holds `stance_torques`, `cycle_rms_torque`, `winding_rise` and `max_body_mass`.
