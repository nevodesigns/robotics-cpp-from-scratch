# A Rover in a Building: A Plan, a Range Sensor, a Map and a Supervisor, and What Happens When Somebody Steps Out

> A rover that reaches the goal in the empty building is a rover that has not been tested.

**Type:** Capstone
**Time:** about 170 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated building and robot)
**Prerequisites:** 21-01, 20-12, 19-14, 16-07, 16-06, 16-05, 16-04, 19-05, 19-10

## The Problem

The second capstone is the robot that most people mean by "autonomous": it has a plan of a building, a goal, a sensor, and it drives. Everything it needs was built in earlier phases: the planner of lesson 16-06, the map of 16-05, the pursuit of 16-04, the inflation of 16-07, the limiter of 19-05 and the supervisor of 19-10. This lesson is the **wiring**, and the tests are the part to learn from: each puts a thing into the building (a person, a closed way, a dead link, a dead sensor) and says what the rover does, **with and without** each piece of the safety layer.

## The Concept

### The building and the rover

A `Building` is read from a picture: `#` is a wall and anything else floor, 0.25 m a cell, the first row at the top. Two rooms of 5 by 6 m with a door of 1.5 m between them, a start in one corner of the first room and a goal in the far corner of the second.

The rover has a **nine beam range sensor** from 60 degrees right to 60 degrees left, reading up to 3 m by stepping along each beam, and `front_reading` gives the limiter the smallest of the three beams ahead, with a NaN if any of them is. It has a **map** that starts as the plan of the building (every cell seen twice, so that floor and walls are both decided) and is changed by what it sees. It plans (A*, unknown cells blocked) on the map **inflated** by 0.3 m so that the path keeps away from walls, and follows the path by pure pursuit.

Every 0.05 s the supervisor is told: the e-stop is closed; whether the base station's heartbeat is fresh; whether the rover is inside the building's outline; whether the sensor is giving numbers; and the speed that the limiter allows for the range ahead. Its decision caps the speed, and the speed changes by at most 1.5 m/s^2. See `E-CAP-0004`.

### With nobody in it

Through the door and across the second room in **12.3 s**, **9.6 m**, 7 plans, **0 bumps**, top speed 0.80 m/s, and the closest it came to a wall was **0.18 m**. The distance is 9.6 m for a straight line of 9.4: the path bends through the door.

### Somebody steps out

A person (a disc of 25 cm radius) steps from the side into the path, 1 to 2 m ahead of the rover, at 4.2 s, and stays until 12 s.

| rover | result | in contact | closest |
|---|---|---|---|
| limiter and supervisor | reached 22.6 s, 11.6 m | **0** | 0.16 m |
| limiter only | reached 22.6 s, 11.6 m | **0** | 0.16 m |
| supervisor only | reached 22.9 s, 12.0 m | **144 ticks** | 0.02 m |
| neither | reached 22.9 s, 12.0 m | **144 ticks** | 0.02 m |

The two without the limiter were **in contact for 144 ticks of 0.05 s, 7.2 seconds**: the rover pushed against a person until they left, and then went on and reached the goal, which is the worst way for a test to pass. And the third line is the point of the lesson: **the supervisor on its own did nothing**. It decides from what it is told; with no limiter, nothing tells it that there is a person, and its cap is the top speed. A supervisor is not a sensor. See `E-CAP-0005`.

### Somebody in the doorway, where the rover cannot see

A person walks into the doorway at 6 s and stays until 20 s. The rover sees them as it comes near, the way is blocked on the map, and there is no other way, so it waits. Then the person leaves. But the rover is waiting 4 m away, from where the doorway is out of view: **nothing on the map ever changes**.

| rover | result | waiting |
|---|---|---|
| never forgets what it saw | **not reached in 120 s**, at 4.2 m | 2296 ticks |
| forgets after 4 s with no way through | reached **27.2 s** | 301 ticks |

A map is a belief about the past, and a belief nobody can check does not go away. After 4 s with no way through the rover drops what it saw, goes back to the plan of the building, sets off towards the door, **looks again**, and if the doorway is still blocked the beams put the person back on the map, and it waits again. This one reached the goal 7 s after the person left. A rover that waits for a way to open must have some way of finding out. See `E-CAP-0006`.

### A link that is lost

The base station's heartbeat stops at 5 s and returns at 9 s.

| | result |
|---|---|
| supervisor on | **not reached**: stopped for 2300 ticks, cause **link**, still at the end |
| no supervisor | reached 12.3 s: it never noticed |

The link came back at 9 s and the rover **stayed stopped**: the supervisor latches (19-10), and a reset is a person's decision. The rover with no supervisor drove on through a 4 second loss of contact, which is what "no stop path" means, and is the case that lesson 19-02 was written for.

### A sensor that goes dead

The range sensor reads NaN from 3 s, and a person steps out at 4.2 s.

| | result | in contact |
|---|---|---|
| limiter and supervisor | stopped after **2.4 m**, cause **sensors** | 0 |
| limiter only | stopped after 2.4 m, **no fault**, no stop cause | 0 |
| neither | reached at 19.7 s | **144 ticks** |

With both, a stop with a cause that can be read off a log. With the limiter alone, the NaN is read as a blocked path (19-05) and the speed limit is **zero**: it stops, safely, and nothing says why, and it would **start again** the moment the sensor came back. With neither, it drove blind through a person. The safe answer is the first, and the reason for the supervisor in the first place is what it adds over the second: a cause, a latch, and a reset. See `E-CAP-0006`.

## Build It

`parse_building`, `person_at`, `front_reading` and `prior_map` in `exercise/solution.hpp`. The sensor, the mapping, the loop and the wiring to the safety layer are given: they are the integration.

```
rcpp verify 21-02
```

## Use It

`rc/capstone/delivery_rover.hpp` joins `rc::capstone`.

```cpp
const Building b = parse_building(rows);
DeliveryConfig config;
config.inflate = 0.3;
const DeliveryResult r = run_delivery(b, people, {3, 3}, {36, 20}, config, 120.0);
```

## Show It

Put the **table of what each layer does** in the README: the row where the supervisor on its own does nothing is worth more than the row where it works. Show the robot that waits 4 m from a door that has been open for ten seconds, and what you did about it. An interviewer who sees you found that the rover can be stuck by a map that is out of date knows you have run it.

**What this does not do.** It is a simulation: the walls are perfect, the beams have no noise, the robot's position is known exactly (there is no odometry error, no localisation, which is the work of phase 16), the person walks in straight lines and does not avoid the robot, and the rover is a point in the planner with a margin. The sensor is nine beams in one plane at 20 Hz, which a real one beats on range and resolution and loses on glass, black surfaces and sunlight. The limiter looks at the way ahead only, so the rover cannot be stopped by something at its side or behind it. The speed is constant except for slowing, with no curvature limit (19-09). There is no bumper, which is the second, different sensor that lesson 19-07 said a robot near people needs. **A rover that touches people needs more than this**; this is the structure.

## What Breaks First

- **A supervisor with nothing to tell it.** See `E-CAP-0005`.
- **A map that is out of date, and a rover that waits on it.** See `E-CAP-0006`.
- **A stop with no cause, and a restart on its own.** See `E-CAP-0006`.

## Ship It

`rc/capstone/delivery_rover.hpp` holds `Building`, `parse_building`, `Person`, `person_at`, `kBeams`, `kMaxRange`, `beam_angle`, `blocked_at`, `cast_beam`, `front_reading`, `map_scan`, `prior_map`, `cells_to_points`, `DeliveryConfig`, `DeliveryResult`, `gap_to_obstacles` and `run_delivery`.
