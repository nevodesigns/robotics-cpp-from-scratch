id: E-SENSE-0013
title: A stuck-sensor threshold chosen by habit rather than from the sensor
match: how long a healthy parked sensor repeats itself
platforms: linux, windows
teaches: 15-04-the-sensor-that-stopped
---

## Symptom

Either of two opposite complaints, from the same line of code.

A perfectly healthy sensor is reported as stuck whenever the robot stops moving,
so the check gets disabled or its threshold raised until it never fires.

Or a sensor that genuinely froze is never reported at all, because the threshold
was raised until it never fires.

## Cause

Whether repeated readings mean anything depends on the sensor's noise compared
against the smallest step it can report, and that ratio is different for every
device.

The longest run of identical readings from a parked, healthy sensor, over a
hundred thousand samples at 1 mm resolution:

| noise | noise / resolution | longest run |
|---|---|---|
| 0.0500 m | 50 | 3 |
| 0.0100 m | 10 | 4 |
| 0.0020 m | 2 | 6 |
| 0.0010 m | 1 | 11 |
| 0.0005 m | 0.5 | 23 |
| 0.0001 m | 0.1 | 100000 |

At fifty times the resolution, three repeats is the most that ever happened, and
a threshold of five or eight is safe. At a tenth of it, a parked sensor repeats
for ever while working perfectly, and no threshold exists that is both useful
and correct.

The same number copied between two devices is right for one of them.

## Fix

Measure the run length before choosing the threshold. Park the sensor, take a
hundred thousand readings, and look at the longest run. Set the threshold well
above it and write down which sensor it was measured on.

Where the noise is below the resolution, do not use a repeat counter at all. It
cannot distinguish a stuck sensor from a still one, and something else has to
carry the check:

- the device's own timestamp, which is the right answer whenever the protocol
  has one, as in `E-SENSE-0012`;
- a sequence number or sample counter from the device;
- a deliberate excitation, where the system can afford one: a reading that does
  not change when the actuator moves is stuck whatever its noise looks like.

The general shape of this is worth keeping past this particular check. **A
threshold that came from a habit is a threshold nobody measured**, and it will
be wrong on the next device by the ratio between the two.
