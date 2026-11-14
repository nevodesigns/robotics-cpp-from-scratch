# Making the Zeros True: Fitting the Joint Offsets of a Leg From Where the Foot Was Measured, and What It Cannot Fix

> A servo horn pressed on a degree out puts every foot a centimetre out, and one afternoon with a ruler fixes it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-07, 34-02, 28-09

## The Problem

Every number in 34-01 to 34-07 assumes that the joint angle the servo reports is the joint angle the leg has. It is not. A servo's zero is wherever it was when the horn was pressed on the shaft, which is a tooth of the spline (a few degrees) away from where the maker intended. Each of the three joints of a leg has an unknown offset, and every foot place is wrong by the offset times the lever: a degree at 0.2 m is 3.5 mm, and three of them together are centimetres, enough to tilt the body (34-03) and to fight the legs against each other. The remedy is a short, cheap measurement and a fit.

## The Concept

### What is measured

Put the leg in a number of poses (here 12, a spread of leans, hip pitches and knee bends), command the angles, and measure where the foot is (on a jig, or a board ruled in squares, with a ruler or a camera). The measurement has noise, here 2 mm in each axis. See `E-LEG-0021`.

### Gauss-Newton

If the offsets were known, the foot would be at `leg_fk` of the commanded angles plus the offsets. They are not, so start at zero offsets and repeat: predict the foot's place, take the difference from the measured one, and find the correction to the offsets that best removes it, by the normal equations (the sum over poses of the Jacobian's transpose times its Jacobian, and times the difference) with the Jacobian of 34-02. A few passes are enough. Offsets of 2, -3 and 4 degrees were found as 1.95, -3.16 and 4.19 degrees. The foot was 14.6 mm from where it was commanded before the fit and 3.4 mm after (the noise of 2 mm in each axis of the measurements is 3.5 mm in all three, which is what was left): the fit took out everything but the noise. See `E-LEG-0022`.

### When the poses do not fix the offsets

If every pose has the leg straight, the knee's offset cannot be told from the hip's (they move the foot the same way) and the normal equations are singular: the function says false instead of making up numbers. A good set of poses has the leg bent, leaned and swung in different amounts. A single bent pose in fact fixes three offsets from a three-dimensional measurement, but noise then has nothing to average against, which is why a dozen poses are used.

### What offsets cannot fix

If the thigh is 10 mm longer than the geometry says, no offsets explain it: the fit still ran and the residual stayed at 3.6 mm, far above the noise level. A residual much larger than the noise says that the model, not only the zeros, is wrong (a link length, a hip offset). Measure those, or fit them too, and do not accept a calibration whose residual is large. See `E-LEG-0023`.

## Build It

`calibrate_offsets` in `exercise/solution.hpp`.

```
rcpp verify 34-08
```

## Use It

`rc/legged/leg_calib.hpp`.

```cpp
double offsets[3], rms;
if (!rc::legged::calibrate_offsets(leg, commanded, measured, n, 20, offsets, &rms) || rms > 0.004) recalibrate();
```

**What this does not do.** It fits the three zeros only: link lengths, the hip offset and backlash are taken as exact (the test shows what a wrong length looks like). The measurement of the foot is taken as free of bias; a jig that is itself out puts that error into the offsets. The offsets are constants: a horn that slips changes them. The legs of a robot are calibrated one by one, and a robot of twelve joints has twelve offsets, so it takes twelve times the care. The numbers are typical.

## What Breaks First

- **Zeros taken as true.** See `E-LEG-0021`.
- **A calibration accepted with a large residual.** See `E-LEG-0023`.
- **A calibration from poses that cannot tell the joints apart.** See `E-LEG-0022`.

## Ship It

`rc/legged/leg_calib.hpp` holds `solve3_rel` and `calibrate_offsets`. With it phase 34 is complete: a leg modelled well enough to stand on and step with, its geometry both ways, its force and torque, the body's pose from the feet, the timing and the path of a step, the load and heat of its servos, the sense that the foot has touched, and the calibration that makes the numbers true.
