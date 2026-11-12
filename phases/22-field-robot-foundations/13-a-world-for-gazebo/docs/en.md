# A World for Gazebo: The Same Scenario File Made Into an SDF World, With the Ground, the Walls, the Posts, the Goals and the Robot Where They Belong

> The scenario was written for one simulator; a world made from it is the same question asked of another.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-05, 22-07, 22-12, 21-10

## The Problem

The scenario of lesson 22-07 says where the ground is soft, where the walls are, where the robot starts and where it is to go. It was read into the built in simulator. To ask the same question of Gazebo Sim, the same facts must reach it in the language it reads, which is **SDF**, and they must reach it **by being made from the same file**, not by someone typing the walls again. Typed twice, a wall will be moved in one of them, and the two simulators will then disagree about a thing that was never in dispute.

## The Concept

### What a world is made of

`to_world` takes a scenario and a robot description (lesson 22-12) and writes one SDF world:

- the **physics** at a step of 1 ms, not held to the clock (a real time factor of 0: the program that drives it steps time itself, and a run should take as long as the physics needs, which here was about a fifth of the time of the run), and the plugins that make a world run: physics, user commands, and the scene broadcaster that publishes where everything is;
- a **ground slab**, 200 m square and 0.2 m thick, with the friction of the scenario's base surface;
- each **patch** as a slab of its own with its surface's friction, one millimetre higher than the one before, because where two patches overlap the later is on top (lesson 22-05);
- each **wall** as a thin tall box, each **post** as a cylinder;
- each **goal** as a marker you can see and cannot touch;
- the **robot**, from the file, at the start pose with its heading.

The scenario of the test (two patches, two walls, a post, a goal) gave **8 models in 6.8 kB**, all of them fixed except the robot. See `E-FIELD-0039`.

### The two conversions that go wrong

A wall is a segment between two points and Gazebo wants a box. `wall_box` puts the box at the middle of the two ends, makes its long side the length of the segment and turns it by `atan2(dy, dx)`. A wall from (0, 0) to (3, 4) became a box **5.000 m** long turned **0.927295 rad** with its centre at (1.5, 2); the same wall written from the other end came out the same length turned half a turn, which is the same wall. A box laid along the x axis whatever the wall's direction is the commonest mistake, and every wall that happens to be axis-aligned hides it. See `E-FIELD-0040`.

A slope is a pair of grades and Gazebo wants angles, with a sign convention. A rotation about y by a positive angle turns the x axis **downwards**, so a surface that rises along +x by a grade of `g` is a pitch of **minus** `atan(g)`: a grade of 0.1 became **-0.0996687 rad**. A grade along +y is a positive roll. The test checks the sign by working out what the rotated slab does to a unit step along x, which is a rise of 0.1. A hill that goes down is as easy to write as a hill that goes up. See `E-FIELD-0041`.

### What is not in the world

The faults of the scenario (a GNSS outage, a lidar dropout) are not in the world. They are about what the **robot's sensors report**, and they are applied in the adapter between the world and the program, which is the next lesson. The world only has what is physical.

### Checking it

The test checks the structure: balanced tags, a model for each thing, the friction and the poses printed as they should be. Gazebo's own parser is the stricter judge, and when this lesson was written the world of the test was accepted by `gz sdf -k` and run in Gazebo Sim 8.15 (Harmonic) headless. To look at it yourself, write the text to `yard.sdf` and run `gz sim -r yard.sdf`.

## Build It

`wall_box` and `patch_tilt` in `exercise/solution.hpp`.

```
rcpp verify 22-13
```

## Use It

`rc/field/gz_world.hpp` opens the module `rc::field`.

```cpp
const rc::field::ScenarioParse s = rc::field::parse_scenario(read_file("yard.scenario"));
const rc::field::RobotParse r = rc::field::parse_robot(read_file("burger.robot"));
write_file("yard.sdf", rc::field::to_world(s.scenario, r.robot));
```

**What this does not do.** A patch is a flat slab: the tilt of a patch that rises in both x and y is the pair of angles and not the exact plane. Patches step up by a millimetre each, so the robot meets a tiny kerb at the edge of every one. The base ground is a single friction; Gazebo combines the friction of the wheel and of the ground in a way that is the engine's own, and how much that differs from the built in model is measured in the lesson at the end of the Gazebo part of this phase and not assumed. Lighting, textures and sensors are not made at all.

## What Breaks First

- **A world typed out again by hand.** See `E-FIELD-0039`.
- **A wall laid along the x axis whatever its direction.** See `E-FIELD-0040`.
- **A slope that tilts the wrong way.** See `E-FIELD-0041`.

## Ship It

`rc/field/gz_world.hpp` holds `WorldBox`, `wall_box`, `patch_tilt`, `put_pose`, `put_friction`, `put_static_box`, `world_colour` and `to_world`.
