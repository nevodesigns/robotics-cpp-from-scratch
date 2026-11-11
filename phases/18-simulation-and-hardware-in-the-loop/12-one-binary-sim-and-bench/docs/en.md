# One Binary, Sim and Bench: The Same Controller and Scenario Against Either, and the Gap Between Them Measured

> The test of a robot's software is not whether it works in simulation, or whether it works on the bench. It is whether the same program does both, and whether you know by how much they differ.

**Type:** Build
**Time:** about 160 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-11, 18-09, 18-08, 03-10, 02-04

## The Problem

The phase has built the parts of a rig. A fixed step loop (18-01); a plant interface with a contract (18-02); a motor model measured from the machine (18-03); a sensor that lies like the real one (18-04); a scenario as a text file (18-05); injected faults and the monitors that catch them (18-06); randomness that is independent per part (18-07); a way to measure how far simulation is from bench (18-08); a bench link with a plant emulator and a controller side (18-09); transcripts (18-10); and a judge that ends a run in pass or fail (18-11).
What is left is to put them together so that the thing is **one program**.

The failure this guards against is the one that ends most simulation projects: two code bases. A controller for the simulation, a different one for the robot, tested separately and diverging for a year. The only way to keep them the same is to have no second one. The controller is a function from measurements to commands, the choice of what is on the other side is **configuration**, made when the program starts, and everything else follows.

## The Concept

### Chosen when it starts

A configuration is a text file of `key = value` lines:

```
backend = bench
link_delay = 2
loss = 0.1        # one in ten
noise = 0.005
watchdog = 8
seed = 7
```

`backend` is required, `sim` or `bench`, and everything else has a default: a link delay of 1, no loss, a watchdog of 5 ticks, a seed of 1. `backend=sim` alone, with no spaces and no final newline, was a valid configuration. See `E-SIM-0034`.

It is read as carefully as the scenario of lesson 18-05. A configuration of 8 lines, every one of them wrong (a backend of `hardware`, a delay of 1.5, a loss of 2, a negative noise, a seed of 0, a setting called `speed`, a line with no `=`, a negative watchdog), gave **8 errors, one for each line**, with line numbers 1 to 8, and left the settings as they were. A configuration with no backend is an error too: "backend is required". A program that starts with half a configuration
is a program whose behaviour no one can describe. See `E-SIM-0034`.

### Two backends, one interface

The controller talks to a `Backend` and nothing else:

```cpp
class Backend {
 public:
  virtual const char* name() const = 0;
  virtual double step(double command, double dt) = 0;     // the position as measured, after one tick
  virtual double truth() const = 0;                       // where the plant really is
};
```

The **simulation backend** is the plant of lesson 18-02 and the sensor of 18-04, directly. The **bench backend** is the plant emulator of lesson 18-09 behind a link that delays and loses frames, with its watchdog, the position read through the same sensor model and sent back up the same link. `make_backend` returns one or the other from the configuration, and both start with the plant at 0 and move the same way when driven. That is the seam of lessons 17-17 and 18-02 once more, now at the
size of a whole program. See `E-SIM-0035`.

### Running a scenario

`run_scenario` takes a backend, a scenario from lesson 18-05 and a gain, runs the proportional controller at 100 Hz through the events, and records the target and where the plant **really** was, from `truth()`, not from the measurement. The controller never learns which backend it has. The trace goes to the tools of lessons 18-08 and 18-11.

### Parity when nothing is wrong

On a **clean** bench, no delay and no loss, the run should be the simulation's. It was: an rms difference of **0.0000**, a largest difference of 0.0000 and a best shift of 0 ticks. And both passed the criteria of lesson 18-11, an overshoot of 0.3, a settling of 1.0 s, a final error of 0.02, with no failures. This is the first test of a rig: that the two backends, set up to be the same, **are** the same. If they differ here the difference is a bug in a backend and not a property of the machine, and nothing measured later means anything. See `E-SIM-0035`.

### The gap, measured

Now the bench is made like a real one: a link delay of 2 ticks each way, 5 percent loss and noise of 0.003. Two runs of it with different seeds differ by **0.0053** rms: that is its noise floor (lesson 18-08), and no simulation can be closer to it than that. The simulation of the same scenario, with no knowledge of the link, was **0.1901** rms away.

The simulation can be told about the link. It has no link, but it has a sensor delay (lesson 18-04), and the round trip of the link is 4 ticks, 2 each way. The simulation's difference from the bench, by its sensor delay:

| sensor delay (ticks) | rms difference from the bench |
|---|---|
| 0 | 0.1901 |
| 1 | 0.1700 |
| 2 | 0.1443 |
| 3 | 0.1104 |
| **4** | **0.0772** |
| 5 | 0.1403 |
| 6 | 0.4375 |
| 7 | 1.0096 |
| 8 | 1.2860 |

The best match was a sensor delay of exactly the link's round trip, 4 ticks, and cut the difference by 59 percent. Past it the difference grows fast, since a simulation with too much delay is a different, unstable loop. The remaining 0.0772 is well above the floor of 0.0053, and the reason is what the simulation still does not have: the 5 percent loss, which a delay cannot imitate. The number says how much simulating a link cost, and what is left to model. See `E-SIM-0036`.

### Repeatable

The same bench configuration run twice gave identical traces, to the last number, and another seed gave a different one; the simulation likewise. Each part draws from its own stream (lesson 18-07): the link's loss in each direction and the sensor's noise are separate, so changing the loss does not change the noise. A scenario, a configuration and a seed are a run, and the run can be sent to someone else. See `E-SIM-0036`.

## Build It

`parse_config`, `BenchBackend::step` and `make_backend` in `exercise/solution.hpp`. The configuration, the interface, `SimBackend` and `run_scenario` are given.

```
rcpp verify 18-12
```

Everything is seeded, so the figures above are the same on every run.

## Use It

`rc/sim/rig.hpp` joins `rc::sim`.

```cpp
rc::sim::RigConfig config;
if (!rc::sim::parse_config(read_file(argv[1]), config).ok) return 2;
std::unique_ptr<rc::sim::Backend> backend = rc::sim::make_backend(config);
const rc::sim::Trace trace = rc::sim::run_scenario(*backend, scenario, 8.0, change_times);
```

**What this does not do.** The bench backend is an emulator: its plant is the same model as the simulation's, which is why the clean bench matches it to four places, and a real bench would be the real motor, with the differences lesson 18-03 measured. The real hardware backend, a driver for a serial port and a motor controller, would be a third class behind `Backend`, checked by the contract of lesson 18-02 and recorded by the transcripts
of lesson 18-10. The controller here is proportional and a real one has state (lesson 14-01). And the program is a function and a configuration: reading a config file, a scenario file and writing a trace to disk is `main`, which is a few lines of what phase 4 built.

## What Breaks First

- **Two code bases.** A controller for each world, and a gap nobody can measure. See `E-SIM-0035`.
- **A configuration that starts with half its settings.** A run no one can describe. See `E-SIM-0034`.
- **A simulation unlike the bench by a number nobody has.** Trust on a feeling. See `E-SIM-0036`.

## Ship It

`rc/sim/rig.hpp` holds `BackendKind`, `RigConfig`, `ConfigResult`, `parse_config`, `Backend`, `SimBackend`, `BenchBackend`, `make_backend` and `run_scenario`.

This ends phase 18: one control binary that runs the same scenario against a simulated plant and a bench rig, with the difference between them measured instead of assumed.
