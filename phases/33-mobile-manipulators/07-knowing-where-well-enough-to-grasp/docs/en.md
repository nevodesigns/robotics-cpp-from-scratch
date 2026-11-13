# Knowing Where Well Enough to Grasp: An Error Budget From Base to Fingertip, the Chance a Grasp Works, and the Camera That Takes the Base Out of It

> Localisation to the centimetre is not enough for a grasp that has to be right to the millimetre; the last stretch has to be measured at the hand.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-06, 33-01, 31-03, 25-06

## The Problem

A mobile manipulator knows where it is from its localisation (25): a **couple of centimetres** and **about a degree** is good. The arm then reaches to where the object is **said to be**. A grasp with a hand whose fingers open **8 cm** onto an object **6.5 cm** wide has **a centimetre of room each side**. The centimetre has to cover **every error from the base to the fingertip**, and 33-01 said that the **heading's error is multiplied by the reach**. This lesson adds them up, says what the **chance of a good grasp** is, and shows what fixes it.

## The Concept

### Independent errors add in quadrature

The base's **position** (standard deviation **20 mm** along an axis), its **heading** (**1 degree**), the arm's **mounting turn** (**0.2 degree**) and the arm's own **repeatability** (**0.1 mm**) are independent, so their variances add. The two angles are **multiplied by the lever** (the distance from the base's centre to the hand) first: at **0.9 m** the degree of heading is 16 mm. The hand's standard deviation along an axis came to **25.6 mm**, **dominated by the base** and not by the arm, which is repeatable to a tenth of a millimetre. See `E-MOB-0019`.

### The chance of a good grasp

If the error is **normal** with standard deviation sigma, the chance that it is within plus or minus a tolerance is the **error function** of the tolerance over sigma times root two. With a tolerance of **10 mm** the chance was **0.30**: about **one grasp in three**. **20000 simulated grasps** gave **0.30**. A robot that grasps by localisation alone, to the centimetre, **fails most of the time**. See `E-MOB-0020`.

### What sigma a chance needs

**1.96 sigmas** make **95 percent** and **2.58** make **99**. For 99 percent with a 10 mm tolerance the hand's sigma has to be **3.9 mm**, **six times better** than the budget above. It cannot be had by a better heading alone, since the position error of 20 mm dominates; the base would have to be known to **millimetres**, which a lidar localisation (25) does not give.

### Measure at the hand

A **camera on the hand** (31-03) measures the part **relative to the hand**, so **none of the base's errors** (position, heading, mount) are in the measurement: what is left is the **camera's error at that range** (**2 mm**) and the arm's repeatability. The sigma fell to **2.0 mm** and the chance to **1.000**. The base only has to stop **somewhere in the ring** (33-02) and the camera does the rest. This is **why a mobile manipulator has a camera on the arm**, and the same reasoning shows when it can be left out (a fixture that locates the part to a millimetre). See `E-MOB-0021`.

## Build It

`hand_sigma` in `exercise/solution.hpp`.

```
rcpp verify 33-07
```

## Use It

`rc/mobile/grasp_budget.hpp`.

```cpp
const double s = rc::mobile::hand_sigma(sigmas, lever);
if (rc::mobile::grasp_probability(s, tolerance) < 0.99) use_the_camera();
```

**What this does not do.** Errors are **independent and normal**, with the **same sigma in x and y**: the real ones are correlated (the base's position and heading errors come from the same filter, 25-06) and have heavy tails. The **tolerance** is a single number along one axis; a real grasp has tolerances in several directions and in orientation. The **camera's** error is a **single sigma at one range**, and its calibration (31-03's transform) is assumed exact; its own error against the hand would add in. The numbers are **typical**. **Time** is not here: the base may still be moving, or settling, when the camera looks.

## What Breaks First

- **A grasp planned from the map alone.** See `E-MOB-0020`.
- **A heading error counted as a position error.** See `E-MOB-0019`.
- **A camera error taken from the sheet at the wrong range.** See `E-MOB-0021`.

## Ship It

`rc/mobile/grasp_budget.hpp` holds `Sigmas`, `hand_sigma`, `grasp_probability`, `z_for_probability`, `sigma_needed` and `sigma_after_servo`.
