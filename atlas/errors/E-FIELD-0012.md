id: E-FIELD-0012
title: An encoder on the motor trusted to say where the output is
match: the output shaft is a degree off after a move and the encoder says it is exact
match: the same command leaves the robot in different places depending on the direction it came from
match: on reversing, the gear side crosses the whole gap before the output moves
match: a position loop on the motor encoder settles where the encoder says, and the output is off by half the gap, on the side it came from
platforms: linux, windows
teaches: 22-04-motors-gearboxes-and-backlash
---

## Symptom

A position that is exact in the log is a little off on the robot, and the error changes sign with the direction of approach. It is a gearbox with some play.

## Cause

The encoder is on the motor's side of the gearbox, and the gear has play. The output rested at the edge of the gap: -1.00 degree approached from below and 1.00 degree from above.

Between the motor and the output there is a gap in the gears. The motor's encoder measures the motor; the output may rest at either side of the gap depending on which way it last moved.

## Fix

Put a sensor on the output, or always approach a position from the same side, or model the gap.

```cpp
const double where = motor.output_angle();   // not the motor's angle through the ratio
```
