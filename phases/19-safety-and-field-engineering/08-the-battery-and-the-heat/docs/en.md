# The Battery and the Heat: Derating Before Stopping, Voltage That Sags Under Load, and a Resume Point Above the Stop Point

> A robot that stops when its battery is empty has done nothing clever. A robot that stops while the battery is half full, every time it accelerates, has an unsafe monitor.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-07, 19-01, 18-07, 01-06

## The Problem

G5 in lesson 19-01 was "battery fails in the field", scored 8, and G6 "motor overheats and burns", scored 6. They look like maintenance problems and they are safety problems, because of what happens when they are met in the middle of a task: a robot that dies at the top of a slope, or on a road crossing, or with an arm extended, is a hazard, and one whose battery catches fire is a worse one.

Two things go wrong in the **monitor**, and not in the battery. A monitor that **cuts out** where it should have slowed has made the failure sudden, when it could have been gradual. And a monitor that watches the voltage at the terminals is measuring the load as much as the battery.

## The Concept

### Slow down before you stop

`PowerMonitor` gives a **share of full power**, from 1 down to 0, and not a yes or no. The share falls linearly from 1 at the warning voltage (22.0 V) to 0 at the stop voltage (20.0 V), and in the same way from 1 at 60 degrees to 0 at 80. The smaller of the two wins.

| at rest, 25 degrees | share of power | state |
|---|---|---|
| 24.0 V | 1.00 | ok |
| 22.0 V | 1.00 | ok |
| 21.5 V | 0.75 | derate |
| 21.0 V | 0.50 | derate |
| 20.5 V | 0.25 | derate |
| 20.1 V | 0.05 | derate |

At 70 degrees the share was 0.50, and at 80 it stopped. A robot that has been told to go at half its speed has time to get somewhere safe. A robot that has been stopped has not. The warning zone is also the monitor telling the rest of the system, early, that the end is coming. See `E-SAFE-0022`.

### The voltage at the terminals is not the charge

A battery has an internal resistance, and the voltage at its terminals sags by current times resistance while the load is drawn. A battery at 22.0 V at rest and a resistance of 0.1 ohm, drawing 15 A, reads **20.5 V**. It has lost nothing: when the load goes, the voltage comes back.

A battery monitor that stops below 21 V at the terminals stopped the robot at the start of every second of a load that pulsed 15 A on and off for 60 s, and let it go again at the end: **60 changes** between stopped and running in a minute, on a battery that was fine. The monitor here adds the sag back (`rest = volts + amps * resistance`) and judged the battery at 22.0 V throughout: **0 changes**. A robot that stops under load is a robot that stops when it has to climb. See `E-SAFE-0023`.

Regeneration is not a load: a negative current does not add a correction, because a battery being charged reads **high**, not low.

### A resume point above the stop point

A battery sitting right at its stop voltage, 20.00 V with a noise of 0.05 V, crosses the threshold on every other sample. With a resume point **equal** to the stop voltage the monitor stopped **135 times** in 600 samples. With a resume point of 21.0 V it stopped **once** and stayed stopped, until the battery was seen at 21.2 V. This is **hysteresis**: the point to start again is not the point at which it stopped. Without it, a robot at a threshold twitches, and a motor driver or a contactor switched 135 times in a minute does not survive. The same goes for the temperature, which resumes at 70 degrees after a stop at 80. See `E-SAFE-0023`.

### A motor that heats while it works

A motor that heats at 1.2 degrees per second at full power (in proportion to the power squared) and loses 0.02 of its excess over 25 degrees per second, held at full demand for an hour at 10 Hz:

| | peak | stops | mean power, second half |
|---|---|---|---|
| derating | 63.9 degrees | 0 | 0.81 |
| hard stop only | 80.0 degrees | 53 | 0.85 |

The derating monitor found a power where heating and cooling balance, at about **0.81**, and never stopped. The hard stop ran at full power until it hit 80 degrees, stopped, cooled to 70, ran again, and did it **53 times** in the hour, with a peak at the limit every time. The mean power was almost the same (0.85 against 0.81): the hard stop does not give much more work, only a machine that is hotter and keeps stopping, in places where it was not told to. See `E-SAFE-0024`.

### A reading that cannot be real

A NaN, a temperature of 200 or -80 degrees (a thermistor wire that came off reads off the scale), a battery voltage of 2 V (a broken wire) and a current that is not a number all stopped the monitor. A good reading brought it back, because nothing was wrong with the battery or the motor. See `E-SAFE-0024`.

## Build It

`PowerMonitor::update` in `exercise/solution.hpp`. The configuration and types are given.

```
rcpp verify 19-08
```

## Use It

`rc/safety/power.hpp` joins `rc::safety`.

```cpp
rc::safety::PowerConfig config;            // from the battery's data sheet and the motor's rating
rc::safety::PowerMonitor power(config);
const rc::safety::PowerLimit limit = power.update(pack_volts, pack_amps, motor_temp);
const double allowed_command = command * limit.factor;
```

**What this does not do.** The internal resistance is a number to be measured, and it rises as a battery ages and gets cold, so a fixed one gets less true each season. A lithium battery is also protected by its own battery management system, which cuts it off with no regard for your software, and the robot has to survive being cut off by it (lesson 19-03). The model of heating is a toy: a real motor has a winding that heats much faster than its case, and the sensor measures the case. The monitor says **stop** and does not latch it: a state that is stopped on one sample and running on the next is for the supervisor of this phase to latch and to reset, as lesson 19-02 does for the link. And it measures nothing of the **cells of a pack** one by one, the hazard that causes fires.

## What Breaks First

- **A monitor that stops where it could have slowed.** See `E-SAFE-0022`.
- **A cut-off on the voltage at the terminals.** See `E-SAFE-0023`.
- **A threshold with nothing above it to resume at.** See `E-SAFE-0023`.
- **A thermal limit that is only a stop.** See `E-SAFE-0024`.

## Ship It

`rc/safety/power.hpp` holds `PowerState`, `PowerConfig`, `PowerLimit` and `PowerMonitor`.
