id: E-CTRL-0020
title: A dead band wide enough to stop the hunting, and the accuracy it cost
match: freezing the integral inside a small band stops the hunting, and a wide band costs accuracy
match: expected this to be true: results.*final_error
platforms: linux, windows
teaches: 14-08-stuck-then-slipping
---

## Symptom

A fix for hunting that works, and then a complaint that the joint no longer reaches its
target: it stops 3 or 4 centimetres short, every time, and it is quiet. The person who
added the band to stop the oscillation does not connect the two.

## Cause

The simplest way to stop an integral winding a sticking joint into a limit cycle is to stop
integrating when the error is small, a band. Inside the band the loop acts as proportional
only, so the mass sticks and stays. It works, and what it costs is the accuracy of the
target by about the width of the band.

Measured, the loop of `E-CTRL-0018` with an integral gain of 20, the integral frozen while
the error is smaller than a band:

| band | final error | peak to peak | |
|---|---|---|---|
| none | -0.0196 | 0.0397 | hunting |
| 0.01 | -0.0051 | 0.0000 | settled |
| 0.03 | -0.0071 | 0.0000 | settled |
| 0.08 | -0.0367 | 0.0000 | settled, further off than the hunting went |

A narrow band, of the order of what the joint can resolve, stops the hunting at a fraction
of the error the hunting itself had. A wide one is quiet and wrong by more than the
oscillation ever was.

## Fix

Choose the band from what the application can tolerate and the friction allows, and write
the choice down. The band cannot be narrower than the breakaway force over the gain
allows the mass to resolve, or the loop will still hunt, and it should not be wider than the
error that can be tolerated, or it will not reach the target. Other remedies exist and each
has its own price: dither, a small oscillation added deliberately to keep the mass moving, a
stiction compensation, a short kick of extra force to break free, a better actuator or a
position sensor on the output side that sees the stick. The band is the cheap one.
