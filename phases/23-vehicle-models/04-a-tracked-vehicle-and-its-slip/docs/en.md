# A Tracked Vehicle and Its Slip: The Thrust a Track Gets From Soil, the Slip It Costs, and What a Little More Slip on One Side Does to a Straight Line

> A track grips by slipping, and the slip is not a loss that can be avoided but the price of the thrust.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-02, 22-05, 21-10

## The Problem

A track is a wheel with a very long flat patch on the ground, and on soft ground that is what lets a tracked machine go where wheels sink. But a track does not roll on the ground as a wheel does on tarmac; it **shears** it. The soil under the track is pushed backwards a little, and the thrust the track gets is the reaction to that push: no push, no thrust. The push is a slip. So a tracked vehicle driving forward is always slipping, by an amount that depends on how much thrust is needed, and the slip of the two tracks does not have to be the same. A model of a tracked vehicle that has none of this will have its odometry wrong from the first metre and will not be able to say whether it can climb a slope.

## The Concept

### Thrust from slip

For a track of contact length `L`, width `b` and load `W` on a soil with cohesion `c`, internal friction angle `phi` and shear deformation modulus `K`, the thrust at a slip `i` is

`F(i) = (c * A + W * tan(phi)) * (1 - K / (i * L) * (1 - exp(-i * L / K)))`

with `A = L * b` the area in contact. This is the shear curve of Janosi and Hanamoto, used for tracks in the terramechanics literature. It is **zero at no slip**, rises almost linearly at first (with a slope of the largest thrust times `L / (2K)`), bends over and tends to the **largest thrust the soil can give**, `c*A + W*tan(phi)`, as the slip grows: that is the 100 percent mark that no slip can pass. The soils here are **typical** values of a loose sand and a firm soil as given in textbooks, not those of a particular soil, and `E-VEH-0010` is the reason that matters.

On a loose sand a track of 0.40 m, 0.08 m wide and carrying 98 N gave **half its largest thrust at a slip of 10.0 percent**. The curve is steep at the start and flat at the end: most of the thrust is got for the first few percent of slip and the last part costs a great deal. `slip_for_thrust` inverts it. See `E-VEH-0010`.

### The pivot, for a track

A track has its weight spread along its length, not at four points, and a pivot has to scrub all of it sideways. With an even pressure the resistance to turning is `mu * m * g * L / 4`, and the two tracks, one forward and one back, give a torque of the thrust times the gauge. A 20 kg vehicle with tracks of 0.40 m, 0.30 m apart, on a floor of friction 0.5, needed **32.7 N a track**; the four wheeled chassis of the same size of lesson 23-02 would have needed **81.8 N**. The track's pivot is cheaper, because its weight is spread nearer the middle than the wheels, which sit at the ends. On the loose sand, that thrust took a slip of **6.8 percent**. See `E-VEH-0011`.

### Two tracks, two slips

`TrackedDrive` is a differential drive in which each track has its own slip. With both tracks driven at 0.3 m/s and slips of 10 and 20 percent, the vehicle was not going straight: the track that slips more is slower over the ground, so it turned the vehicle towards that side by **44.1 degrees in 10 s**. With equal slips it went straight. A tracked robot that holds a heading by sending equal commands to both tracks will veer wherever the ground differs from one side to the other, a rut or a patch of wet, and a heading must be held by a feedback on a measured heading and not by symmetry. See `E-VEH-0012`.

## Build It

`track_thrust` in `exercise/solution.hpp`.

```
rcpp verify 23-04
```

## Use It

`rc/vehicle/tracked.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::Soil soil;
rc::vehicle::Track track;
const double slip = rc::vehicle::slip_for_thrust(soil, track, thrust_needed);
```

**What this does not do.** The soil is described by three numbers and is the same everywhere; real soil has moisture, layers and a surface that fails under repeated passes. The thrust curve is for a track with an even pressure and no grousers, and takes no account of the sinkage that makes the vehicle drive uphill all the time, or of the bulldozing of the soil in front of the track. The slip is taken as given for the drive, not found from the forces. Treat every number as the shape of an answer to measure, not an answer.

## What Breaks First

- **Soil parameters taken from a book as if they were the soil.** See `E-VEH-0010`.
- **A pivot planned without its thrust.** See `E-VEH-0011`.
- **A heading held by symmetry.** See `E-VEH-0012`.

## Ship It

`rc/vehicle/tracked.hpp` holds `Soil`, `Track`, `max_thrust`, `track_thrust`, `slip_for_thrust`, `pivot_thrust_tracked` and `TrackedDrive`.
