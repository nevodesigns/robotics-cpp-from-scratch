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

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- A fault stays until in.reset (then sit); while up, the link lost is fault 1, the heat at or over 1 is fault 2 (not when sitting), a fall (35-09's kFallen) puts it in fallen, rising or lowering for over move_timeout is fault 4; from sit, rise if the operator wants stand or walk and the battery is ok and the heat under 0.8; rising to stand when stood; stand to lowering if sit is wanted or the battery is not ok, to walk if walk is wanted and the command is not zero; walk back to stand only when stopped; lowering to sit when sat; fallen tries to rise again if wanted and safe, counting the tries and fault 5 after max_tries.

As shipped, all 4 of the 4 tests fail. They are, in the order the suite runs them:

- A day of sit, stand, walk, stand and sit goes through the states in order, and a walk is stopped before it is lowered.
- It refuses to rise hot or low, sits down when the battery runs out, and lowers rather than walking on.
- A lost link, hot knees, a move that never finishes and too many falls are faults that only a reset clears.
- Walking needs both the wish and a speed, and getting up needs both the wish and the conditions.

```
rcpp verify 36-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

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
