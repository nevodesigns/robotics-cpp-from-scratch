# GNSS and the Lever Arm: Latitude and Longitude to Metres, and an Antenna That Is Not Where the Robot Is

> A satellite fix is the position of the antenna, and the antenna is on the roof.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-02, 22-06, 21-10

## The Problem

A GNSS receiver reports **where its antenna is**, as a latitude and a longitude, in degrees. A robot wants to know where it is, in metres, in a frame it can plan in. Two conversions lie between them, and each is easy to get quietly wrong: the degrees must become metres, and the antenna's position must become the robot's.

## The Concept

### Degrees to metres

A degree of latitude is the same everywhere: `R` times the angle in radians, **111195 m** on a sphere of 6 371 km. A degree of longitude is not: the meridians meet at the poles, so a degree of longitude is that times the **cosine of the latitude**. At 52 degrees north it is **68458 m**, which is **38 percent less** than a degree of latitude, and a program that forgets the cosine puts every east-west distance that much too long. `to_local` is the flat map made at the origin's latitude, with `from_local` its inverse, and they round-trip to a micrometre. See `E-VEH-0058`.

The map is flat and the earth is not. Against the great-circle distance (`haversine`), a point 1 km away was **0.036 m** out, one 10 km away **3.55 m**, and one 100 km away **357 m**: good for a field, and not for a country. A robot works in one place and one origin does.

### The antenna is not the robot

The antenna is on a mast or a roof, some distance from the point of the robot that the rest of the software calls its position: the **lever arm**, ahead and to the left in the robot's frame. The antenna's position is the reference point plus the lever **turned by the heading**; so the reference point is `antenna - R(heading) * lever`, which `antenna_to_reference` does.

It matters most when the robot turns. A robot that turns on the spot with an antenna 0.5 m from its centre has an antenna that **circles**: its position wandered **1.00 m** across, and the antenna moved at **0.50 m/s** for a turn of 1 rad/s, while the robot did not move at all. A filter that takes the antenna's position for the robot's sees a robot that jumps about, and a velocity that is wrong by the lever times the turn rate. With the lever arm subtracted, the reference point was still to 1e-9. See `E-VEH-0059`.

### The lever arm needs the heading

The correction uses the heading, and the heading is not exact. With it wrong by 5 degrees the residual was **0.044 m**, the lever times the heading error. A long lever and a poor heading give back a good part of what the correction was meant to remove. The lever should be as short as the antenna's view of the sky allows, and the heading as good as the gyro of the last lesson can make it. See `E-VEH-0060`.

## Build It

`antenna_to_reference` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The antenna is at the reference point plus the lever turned by the heading (x' = cos h lx - sin h ly, y' = sin h lx + cos h ly); take that off.

As shipped, 1 of the 4 tests fail, and these are the ones your code has to turn green:

- The antenna is not the robot: it circles when the robot turns on the spot, and the lever arm brings the point back.

The other 3 pass already, because they test the parts that are given.

```
rcpp verify 25-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/locate/gnss_frame.hpp` opens the module `rc::locate`.

```cpp
rc::locate::LocalFrame frame{52.0, 5.0};
double east = 0.0, north = 0.0, rx = 0.0, ry = 0.0;
rc::locate::to_local(frame, fix.lat, fix.lon, east, north);
rc::locate::antenna_to_reference(east, north, heading, rc::locate::Lever{0.4, 0.3}, rx, ry);
```

**What this does not do.** The earth is a sphere, where it is an ellipsoid, and the conversion differs from a survey's by tens of metres a degree; a robot that must match a map made by a survey uses the datum of that map. The lever arm is in the horizontal plane, and a robot that pitches and rolls moves the antenna a little more. The origin is a point chosen by the programmer. And a local frame has no idea which way is north on the robot: that is the heading.

## What Breaks First

- **A degree of longitude taken as a degree of latitude.** See `E-VEH-0058`.
- **An antenna position used as a robot position.** See `E-VEH-0059`.
- **A lever arm corrected with a poor heading.** See `E-VEH-0060`.

## Ship It

`rc/locate/gnss_frame.hpp` holds `LocalFrame`, `to_local`, `from_local`, `haversine`, `Lever` and `antenna_to_reference`.
