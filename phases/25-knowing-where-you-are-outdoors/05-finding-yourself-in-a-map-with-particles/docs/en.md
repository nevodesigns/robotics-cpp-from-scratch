# Finding Yourself in a Map With Particles: A Cloud of Guesses, Weighed by What the Lidar Would See From Each, and the Room That Looks the Same From Two Places

> When you do not know where you are, keep many answers and let the world vote.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-04, 22-06, 24-09, 21-10

## The Problem

The matching of the last lesson needed a first guess, and said nothing about where the robot is on a map: it said how it moved. To find the position on a map that is known, from nothing, the robot has to try everywhere. A robot in a building could be anywhere in it, facing any way, and no single estimate with an error bar describes that. What does is a **cloud of guesses**, each a possible pose, that can be many places at once, and a way to let the world's evidence decide which are right.

## The Concept

### Particles

`MonteCarlo` holds a set of particles, each a pose (x, y, heading) and a weight. It is started spread over the whole room with any heading. Each step does three things.

**Predict.** Every particle is moved by what the odometry says, with noise of its own, so the cloud spreads out as the robot's uncertainty does.

**Correct.** Each particle is weighed by how well the ranges the lidar *would* see from that pose in the map agree with the ranges it *did* see. For each beam the likelihood of the range is a hit, a Gaussian about the expected range, plus a uniform share, since anything can be seen at any range (a person walks by). The uniform share is what stops one wild beam from giving a good particle a weight of zero. A particle's weight is multiplied by the likelihoods of all the beams, and the weights are normalised. Particles that see what the robot saw become heavy. `correct` also returns the **average likelihood** of the scan under the cloud, weighted by the weights the particles had: when the robot is where the cloud says, it is high, and when it is not it falls, which is the signal the last lesson of this phase uses to notice that the robot has been moved.

**Resample.** If the weights have become so uneven that few particles count (`effective_count` under half the number), draw a new set in proportion to the weights, by the low variance method, which keeps the number and makes the weights equal. The heavy particles are copied and the light ones are lost.

### From nowhere to the robot

From a cloud spread over a room of 10 by 6 m, 3000 particles found the robot, to **0.021 m**, after **5 steps** of 0.2 m, with a spread of the cloud under 0.4 m. A robot that drives in the room and sees its walls knows where it is in under 8 s. See `E-VEH-0067`.

### How many

A cloud has to be thick enough that some particles start near the truth. With 50 particles, of 10 runs with different seeds the robot was lost **10** times (the cloud settled somewhere else); with 3000, **0**. The cost of the particles is the beams times the walls, for each: 3000 particles of 24 beams in a room is fine, and a hundred thousand in a building is a problem. See `E-VEH-0068`.

### The room that looks the same from two places

A rectangular room with nothing in it looks the same turned half a turn: the robot at (2, 2) sees what a robot at (8, 4) facing the other way sees. The cloud settled on both, and then on whichever the noise favoured. In **2** of 8 runs it ended on the mirror image. With a partition at one side and a post in one corner, which break the symmetry, it was **0**. The map decides what can be found, and no number of particles tells apart two places that the sensor cannot. See `E-VEH-0069`.

## Build It

`MonteCarlo::correct` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each beam, the likelihood of the range seen is z_hit times a Gaussian of the difference from the expected range (the map's range from this particle; max_range for none) plus z_rand over max_range; the weight is multiplied by exp(temper times the sum over the beams of the log likelihood).

As shipped, 4 of the 5 tests fail, and these are the ones your code has to turn green:

- From a cloud over the whole room three thousand particles find the robot.
- Too few particles lose the robot more often than enough do.
- In a room that is the same from two places the cloud settles on both, and a feature in one corner picks one.
- Resampling keeps the number of particles and equal weights, and does not happen when the weights are even.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 25-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/locate/mcl.hpp` opens nothing new in `rc::locate`.

```cpp
rc::locate::MonteCarlo mc(map, 3000, 1);
mc.init_uniform(0.0, 10.0, 0.0, 6.0);
mc.predict(ahead, turn, 0.02, 0.02);
mc.correct(beam_angles, ranges, rc::locate::McMeasure());
mc.resample_if_needed();
const rc::locate::Particle where = mc.mean_pose();
```

**What this does not do.** The map is walls and posts that do not move, and the weights are the product of beams taken to be independent, which they are not: neighbouring beams hit the same wall, and the cloud becomes too sure of itself, so the log likelihood is scaled down by `temper`. The mean of a cloud with two modes is between them, which is nowhere; a robot needs to look at the spread, and at the modes. It has no way to recover when the cloud has collapsed on the wrong place (the last lesson of this phase). And each particle is a full ray-cast of the map, which is the cost.

## What Breaks First

- **A cloud that starts in the wrong place.** See `E-VEH-0067`.
- **Too few particles.** See `E-VEH-0068`.
- **A map that is the same from two places.** See `E-VEH-0069`.

## Ship It

`rc/locate/mcl.hpp` holds `Particle`, `McMeasure` and `MonteCarlo`.
