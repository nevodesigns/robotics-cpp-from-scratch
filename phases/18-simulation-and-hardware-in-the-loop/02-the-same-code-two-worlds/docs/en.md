# The Same Code, Two Worlds: A Plant Interface and the Contract a Fake Must Keep

> A fake that has never been compared with the thing it fakes is a guess with a class name.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-01, 03-10, 02-04, 02-08

## The Problem

A controller is tested in simulation because the real machine is expensive, dangerous or in another building. The tests pass, and the controller goes to the real machine, and does something the simulation never did. The usual reason is not that the controller was wrong. It is that the simulated machine did something the real one does not: it did not clamp its command, or it reported where it had just been, or it behaved a little differently in one direction.

The remedy has two halves. The controller must not know which it is talking to, so that nothing in it changes between the two. And the thing it talks to, simulated or real, must **keep the same promises**, written down as rules and checked by a program.

## The Concept

### An interface and nothing else

```cpp
class Plant {
 public:
  virtual void reset() = 0;
  virtual void apply(double command, double dt) = 0;
  virtual double position() const = 0;
  virtual double velocity() const = 0;
  virtual double limit() const = 0;
};
```

A controller takes a `Plant&`. A simulated motor is one class that implements it, a replay of a recorded run is another, and a driver for a real motor over a serial port is a third. This is the seam of lesson 17-17 again, for hardware: nothing in the controller names a simulator.

### A simulated motor

The first implementation is a motor with a lag. Its speed follows `gain` times the command with a time constant `tau`, the command is clamped to the limit, and the position follows the new speed. A motor of gain 2 and time constant 0.1 s, given a command of 3 and stepped at 1 ms:

```
speed 3.804 after 0.1 s (63.4 percent of 6.0), 5.961 after 0.5 s
```

63 percent after one time constant and 99 percent after five: the textbook first order response, which is what makes `tau` a number one can measure on a real motor, and the next lesson does. A command beyond the limit does exactly what the limit does, in either direction: positions after 100 steps at 12 and at 500 are equal, and a command of -12 and one of -500 likewise. See `E-SIM-0005`.

### The contract

A fake is trusted by checking it. `check_contract` takes a way to make a new plant and runs six rules, returning the name of each rule that was broken:

| rule | what it checks |
|---|---|
| reset | after being driven and reset, position and velocity are exactly zero |
| rest | a zero command from rest changes nothing |
| saturation | ten times the limit does exactly what the limit does |
| symmetry | the opposite command gives the opposite position |
| determinism | two new plants given the same commands are in the same state |
| consistency | over one step the position moves by the new velocity times the step |

The simulated motor broke none, with a gain of 2 and a limit of 12, and with a gain of 0.5, a time constant of 2 s and a limit of 1. A contract that nothing could fail would prove nothing, so the test also checks that it finds fault with a plant that has one. See `E-SIM-0004`.

### What each careless plant gives away

Five plants were written as a careless person might, each with one fault. The contract named each:

| plant | broke |
|---|---|
| no clamp | saturation |
| reports where it was before the step | consistency |
| gives a different answer from each new instance | determinism (and, since its bias is there from the start, several more) |
| forgets its speed when reset | reset |
| does less in one direction | symmetry |

Each of these is a mistake that passes any test of a controller on a good day. A plant that does not clamp lets a controller drive it at 80 when the real motor can do 24, and every gain tuned against it is wrong. A plant that reports the position before the step makes a controller a step late, which looks like an unexplained delay. See `E-SIM-0004`.

### The same controller, different plants

A proportional controller, asked for position 1, starting with a command of 40, which is more than the motor may act on:

| plant | ends at | peak speed |
|---|---|---|
| keeps the contract | 1.0000 | 14.70 |
| no clamp | 1.0000 | 22.71 |
| another of the first kind | 1.0000 | 14.70 |

Both reach position 1, and the plant that does not clamp gets there while going **54 percent faster** than the motor can. The end position is the one thing a lazy test checks, and it is the same. The peak speed is what a real motor, a real gearbox or a real person standing nearby would notice. Two plants of the first kind agreed to the last bit. See `E-SIM-0005`.

## Build It

`apply` of `SimMotor` and `check_contract` in `exercise/solution.hpp`. The interface, the constructor and the accessors are given.

```
rcpp verify 18-02
```

Nothing is random or timed.

## Use It

`rc/sim/plant.hpp` joins `rc::sim`.

```cpp
const auto broken = rc::sim::check_contract([]() { return std::unique_ptr<rc::sim::Plant>(new MyDriverPlant(port)); });
if (!broken.empty()) report(broken);
```

**What this does not do.** The contract is a floor: it checks that a plant is sane, not that it is the machine it stands for. A motor with the wrong time constant keeps every rule, and finding that out is the next lesson in this phase. The rules assume a plant whose position follows its velocity, as a motor does, and other plants have rules of their own. A real driver has things a simulation does not, such as a port that can be unplugged, which the contract does not
touch. And the same rules run against a real motor need it to be safe to run them: a saturation test drives it at its limit.

## What Breaks First

- **A fake nobody checked.** A controller tuned against behaviour the real machine does not have. See `E-SIM-0004`.
- **A plant that does not clamp.** Speeds and forces the hardware cannot reach. See `E-SIM-0005`.
- **A test that checks only where it ended.** Two plants that end together and get there differently. See `E-SIM-0005`.

## Ship It

`rc/sim/plant.hpp` holds `Plant`, `SimMotor` and `check_contract`.

A controller can now be written once against a promise, and the promise can be tested against whatever it is given.
