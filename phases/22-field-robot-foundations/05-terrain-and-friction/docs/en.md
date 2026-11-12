# Terrain and Friction: Surfaces, Slopes, and What the Ground Lets a Vehicle Do

> A vehicle can only push on the ground as hard as the ground pushes back, and that is the number every controller above it has to respect.

**Type:** Build
**Time:** about 70 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-03, 22-04, 21-10

## The Problem

A vehicle's limits from lesson 22-03 are what it asks of its motors. The ground has its own, and they are not the same: a wheel can push forward only as hard as friction lets it, and no motor changes that. So the same command that stops a robot in 0.25 m on tarmac stops it in a metre and a half on gravel in the wet, and a slope that a motor could climb is one a tyre cannot. A simulator that has one flat floor gives a controller no way to learn this.

## The Concept

### A surface is two numbers

Friction `mu` decides the most force a tyre can put on the ground (the weight on it times `mu`), and rolling resistance `crr` is what the ground takes back as the wheel rolls. `Surface` holds both and a roughness for later lessons. The five in the header (tarmac, gravel, grass, sand and ice) are **typical** values for a rubber tyre, written as such in the code: a real vehicle on a real field has numbers to measure, and a method for doing it comes with the track in phase 23.

### What the ground allows

On flat ground the most a vehicle can brake is `g * (mu + crr)`, and at 2 m/s the stop took **0.250 m on tarmac, 0.416 m on gravel and 1.853 m on ice**: the last is **7.4 times** the first. The distance went as the inverse of the friction, and no speed limit set for the dry floor is right for the wet one. See `E-FIELD-0013`.

### A slope changes the sum

A slope has a direction. The gradient along the heading is `grade_x * cos(heading) + grade_y * sin(heading)`, and uphill ahead it takes weight off the traction and adds a share of gravity against the drive. Up a grade of 0.2 on gravel the most the vehicle could speed up was **2.020 m/s2**; across the same slope, **4.022 m/s2**, the same as on the flat. Downhill the braking is the weaker: the stop from 2 m/s took **0.717 m** where on the flat it took **0.416 m**. See `E-FIELD-0014`.

### The steepest slope

Drive goes to zero when `tan(angle) = mu - crr`. Gravel can be climbed up to **22.3 degrees** and ice to **5.1**, with all four wheels driven and the weight spread evenly (a two wheel drive vehicle, or one with a rear weight bias, has less). To stay put with the brakes on, the slope may not exceed `mu`: grass holds up to **19.3 degrees**. A planner that sends a robot up a slope its surface cannot be climbed on is not ambitious, it is wrong. See `E-FIELD-0015`.

### Patches

A `Terrain` is a base cell with patches laid on it, each a rectangle of one surface and one slope. A later patch is above an earlier one, and a patch owns its low edges and not its high ones, so two patches side by side never both claim the line between them. A sand pit with a strip of ice across it is two patches.

## Build It

`traction` in `exercise/solution.hpp`.

```
rcpp verify 22-05
```

## Use It

`rc/field/terrain.hpp` opens the module `rc::field`.

```cpp
rc::field::Terrain world;
rc::field::Patch pit;
pit.x0 = 5; pit.y0 = 0; pit.x1 = 10; pit.y1 = 10;
pit.cell.surface = rc::field::sand();
world.add(pit);
const rc::field::Traction t = rc::field::traction(world.at(x, y), heading);
```

**What this does not do.** A patch is a flat plane, so there are no bumps, ruts or steps; the roughness is a number carried for the lessons on traversability. Friction is one number where a real tyre's depends on the slip, the speed and the wetness. The climb limit assumes all wheels share the weight and drive, and the slope is not allowed to change the weight on a wheel from front to back.

## What Breaks First

- **A speed limit chosen for one surface.** See `E-FIELD-0013`.
- **A slope treated as the same in every direction.** See `E-FIELD-0014`.
- **A planner that sends a robot where it cannot climb.** See `E-FIELD-0015`.

## Ship It

`rc/field/terrain.hpp` holds `Surface`, the five surfaces, `Cell`, `Patch`, `Terrain`, `Traction`, `traction`, `max_climb_angle` and `stopping_distance`.
