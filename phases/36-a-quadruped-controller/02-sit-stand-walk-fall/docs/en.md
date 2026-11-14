# Sit, Stand, Walk, Fall: A State Machine That Moves Between Them Only When It Is Safe, and Refuses When It Is Not

> Most of what a quadruped controller does is decide what it may do next, and the most important states are the ones where it does nothing.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-01, 35-09, 34-06, 31-02

## The Problem

A quadruped has modes: it sits, it stands up, it stands, it walks, it sits down, and sometimes it falls. Each mode has things it may do and things it must not, and most of the controller's work is deciding what it may do next. A robot told to walk while hot, low on battery or fallen is a robot that damages itself. The structure is the same as 31-02's pick and place and 33-08's fetch and carry: a state machine with a check at every step, and an interlock that does not trust the operator.

## The Concept

### The states

Sit, rising, stand, walk, lowering, fallen and fault. The operator wants sit, stand or walk. The robot rises from a sit only if the operator wants more than a sit and the battery is enough and the knees are cool (under 0.8 of their limit, 34-06) and the link is alive. It goes to stand when the legs say they have reached the stand, and to walk when walk is wanted and the command is not zero. A day of sit, stand, walk, stand and sit went through 6 changes in order, rising in 0.9 s. See `E-LEG-0054`.

### Wait for the legs

A transition that moves the body waits for the move to finish: rising until the stand is reached, lowering until the sit is. And a walk is stopped before it is lowered: told to sit while walking at 0.6 m/s the machine stayed in the walk until it had slowed to a stop and then went on. A robot that lowers at speed falls. See `E-LEG-0055`.

### Refusing

A robot with knees at 0.85 of their limit refused to rise (1, 1 for yes), and one whose battery was not enough refused (1). A robot standing whose battery ran out lowered itself rather than walk on until it fell. A refusal is a state that does nothing: sit stays sit.

### Faults

A fault stays until it is reset: the link lost while up (fault 1), the knees at their limit (2), a move that took too long (a rise that never finished, 4), and a robot that fell 3 times and still tried (5: out of tries). Every move has a time limit and getting up is limited to a number of tries: a robot that tries for ever wears out its motors on a broken leg. After a fault the machine goes to sit, not to its old state, so the operator sees the reset. See `E-LEG-0056`.

## Build It

`dog_step` in `exercise/solution.hpp`.

```
rcpp verify 36-02
```

## Use It

`rc/legged/dog_state.hpp`.

```cpp
rc::legged::Dog dog;
rc::legged::dog_step(dog, sensed_and_asked, dt);   // every control tick
```

**What this does not do.** The legs' motion between states (the rise and the lowering, and how they are done) is not here: the machine is told when they have finished. Getting up from a fall is a call to a routine (36-03 checks that it can and how long it takes). The walking itself (the gaits of 35, the command of 36-01) is one state. The thresholds (0.8 of the heat, four seconds for a move, three tries) are numbers chosen here. One operator, one robot: no handover between a person and a planner, no e-stop (36-06). The numbers are typical.

## What Breaks First

- **A mode change with no check.** See `E-LEG-0054`.
- **A lowering at speed.** See `E-LEG-0055`.
- **Getting up with no limit on the tries.** See `E-LEG-0056`.

## Ship It

`rc/legged/dog_state.hpp` holds `DogState`, `Want`, `Dog`, `DogIn`, `dog_enter` and `dog_step`.
