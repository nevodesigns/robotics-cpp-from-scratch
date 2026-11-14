# How Far It Can Go and How Hot It Gets: The Energy of the Way Home, the Computer That Never Sleeps, and a Slope That Is Fine for a Minute

> The energy to get home is the energy of the way home over that ground, and the motor has a clock on it too.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-01, 26-09, 19-08

## The Problem

Lesson 21-07 sent a delivery robot home when its battery fell short of the way back, and 19-08 watched a battery and a motor and cut the power when they were in trouble. Both left out what makes a field vehicle's energy and heat hard: the way home is over ground, the computer does not stop when the wheels do, and a motor on a slope has a clock on it.

## The Concept

### The energy of a route, over its ground

The energy of a route is the work against rolling resistance and the slope along each step (lesson 26-09: `m g (crr cos a + sin a)`), divided by the efficiency of the drive (0.7 here), plus the computer and the sensors, which draw their power all the time: 8 W for the time the route takes. A metre of tarmac cost 10.1 J for a 10 kg vehicle at 1 m/s, of which the computer was 79 percent; a metre of sand cost 22.0 J. Speed matters twice: a slower vehicle does the same work and the computer runs for longer. See `E-VEH-0121`.

### The way home is over that ground

A vehicle turns back when what is left would not cover the way home and a reserve. The way home has to be worked out over the ground it will cross. A vehicle that took it as flat turned back 16.8 m out, because the way home looked cheap, and was 25 J short of getting home over the sand band in between. One that planned the way home over the terrain (the planner of 26-09 and the energy of the route) turned back at 12.0 m and got home with 71 J to spare. (A test battery of 0.14 Wh is used to keep the field small; scale the battery and the field together.) See `E-VEH-0122`.

### The motor on a slope has a clock

A slope asks a force `m g (sin a + crr cos a)`; each of the four motors gives a share of it through its gearbox, and carries the current for it (`I = F r / (n eta kt)`); the winding heats as `I squared R`. Its temperature rises towards `P times Rth` with a time constant `Rth times Cth`: that is a single lump, and the time to a limit has a closed form: `-Rth Cth ln(1 - limit / steady)`, or never if the steady rise is under the limit.

For the reference vehicle (the motor values are typical, not measured) a slope of 14 degrees gave a steady rise of 71 K, under the limit of 75 K, and could be held for ever. A slope of 20 degrees reached the limit in 62 s: at 0.5 m/s that is 31 m of climb, and then the motor must rest or the vehicle must derate (19-08 does the derating). A slope that is fine for a minute is a trap for a route that is 100 m of it. See `E-VEH-0123`.

## Build It

`route_wh` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Path_energy over the efficiency, plus the idle power times the time (length over speed); then divide by 3600 for watt hours.

As shipped, 2 of the 5 tests fail, and these are the ones your code has to turn green:

- A metre costs the work, over the efficiency, plus the computer that never sleeps.
- The way home estimated as flat strands a vehicle that the way home planned over the ground does not.

The other 3 pass already, because they test the parts that are given.

```
rcpp verify 27-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/drive/range.hpp`.

```cpp
const double home_wh = rc::drive::route_wh(terrain, path_home.points, power);
if (rc::drive::must_turn_back(soc_wh, home_wh, reserve_wh)) go_home();
```

**What this does not do.** The state of charge is given: working it out from a voltage under load is 19-08's job and a good deal harder than it looks (a cold battery, an old battery). The efficiency and the idle power are constants, and a real drive is less efficient at low load and when it turns (the skid steer of 26-06 scrubs its wheels). A turn costs nothing here. The thermal model is one lump, with one air temperature, no fan, no sun, and the motor starts cold. The motor constants are typical values, not a measured motor, and a different motor needs its own. The way home is planned once at each decision, at the speed given, without regeneration on a downhill. The reserve is a number you choose.

## What Breaks First

- **An energy that forgets the computer.** See `E-VEH-0121`.
- **A way home that is a straight line.** See `E-VEH-0122`.
- **A slope that is fine for a minute.** See `E-VEH-0123`.

## Ship It

`rc/drive/range.hpp` holds `DrivePower`, `route_wh`, `must_turn_back`, `slope_force`, `motor_current`, `MotorThermal`, `steady_rise` and `time_to_limit`.
