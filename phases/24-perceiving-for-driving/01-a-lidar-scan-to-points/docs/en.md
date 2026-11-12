# A Lidar Scan to Points: Ranges Become Points in the Sensor's Frame and Then in the Vehicle's, and What the Gaps Between Them Hide

> A scan is a list of distances, and a thing is only seen if the beams are close enough together to hit it twice.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-02, 22-06, 21-10

## The Problem

A lidar returns a number for each beam, and a robot that is to drive needs to know where things are. The first step from one to the other is the one everybody writes in a few lines and nobody checks: turn each range and angle into a point. Three things go wrong in it, all quietly. The beams that returned **nothing** are written as a number (lesson 22-06 said what the number should be), and become points at the maximum range, which are walls that are not there. The points are in the **sensor's** frame, and a sensor that is tipped down puts a flat wall at different heights. And the points are **not close together**: how far apart they are decides what can be seen at all.

## The Concept

### From ranges to points

`scan_to_points` takes a `LidarScan`, a fan of ranges from `angle_min` in steps of `angle_inc`, with the limits the sensor reports, and gives a point for each beam whose range is a number between the limits. Each point is the range times the cosine and the sine of the beam's angle, in the sensor's frame. A scan of a room of 360 beams gave **360 points**, and the beam straight ahead was at **x = 5.000** on a wall 5 m away. A beam that is infinity, not a number, nearer than the minimum or farther than the maximum gives nothing: `valid_fraction` is the share that gave a point, and it is the first thing to look at when a scan seems wrong. With a tenth of the beams lost the scan gave **325** points, about 90 percent. See `E-VEH-0028`.

### In another frame

The points are in the frame of the sensor, and the vehicle does not care where the sensor is on it. `transform_points` applies a transform of lesson 22-02 to every point. A sensor 0.25 m up and **tipped down 5 degrees** saw the flat wall of a room, a square of 10 m, with heights from **-0.19 to 0.69 m** once in the vehicle's frame: the wall straight ahead is lowered by 5 sin(5 degrees) and the one behind is raised by as much. A planner that takes the points as a flat slice of the world sees a wall that leans. See `E-VEH-0029`.

### How far apart the points are

Neighbouring beams are `angle_inc` apart, so at a range of `r` their points are `r * angle_inc` apart. With one degree between beams, at **10 m the points were 0.175 m apart**. A thing is seen only if at least two beams hit it, which makes the narrowest thing seen at a range `min_detectable_width` and the farthest range at which a thing of a width is still seen `max_range_for_width`: a post 0.3 m wide is hit by two beams at most **17.2 m** away, and beyond that a lidar of one degree sees it only by luck. A sensor's range on a data sheet is the range at which it measures a large flat target, not the range at which it sees a post. See `E-VEH-0030`.

## Build It

`scan_to_points` in `exercise/solution.hpp`.

```
rcpp verify 24-01
```

## Use It

`rc/perceive/scan.hpp` opens the module `rc::perceive`.

```cpp
rc::perceive::LidarScan scan = from_driver(message);
const auto in_sensor = rc::perceive::scan_to_points(scan);
const auto in_vehicle = rc::perceive::transform_points(in_sensor, mount);
```

**What this does not do.** It treats a scan as taken in an instant: a real one takes tens of milliseconds, during which the vehicle moves, and a later lesson of this phase undoes that. The points have no uncertainty, though a range is the less certain the longer it is. The scan is of one plane. And a point is a point: nothing yet says whether it is on the ground, on a wall or on a person.

## What Breaks First

- **A beam that returned nothing, used as a range.** See `E-VEH-0028`.
- **Points left in the sensor's frame.** See `E-VEH-0029`.
- **A sensor's range quoted for a post.** See `E-VEH-0030`.

## Ship It

`rc/perceive/scan.hpp` holds `LidarScan`, `scan_to_points`, `transform_points`, `valid_fraction`, `point_spacing`, `min_detectable_width` and `max_range_for_width`.
