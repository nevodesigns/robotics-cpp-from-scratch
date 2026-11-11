# Goals That Take Time: Feedback, Cancel and the States a Goal Passes Through

> A service answers. A goal takes a while, and in that while the world can change its mind.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-09, 01-06

## The Problem

Lesson 17-09 asked a question and waited for the answer. A robot is asked to do things that take seconds: drive to the loading bay, move the arm to this pose, rotate for a minute and a half. A service that blocks a caller for that long is useless: no way to know how far along it is, no way to stop it, and a timeout that means "I do not know" for 90 seconds.

An **action** is a service that takes time. The client sends a **goal**, gets **feedback** as it goes, may ask to **cancel**, and eventually gets a **result**. The interesting part is not the messages. It is the states, and how many ways there are to get them wrong.

## The Concept

### Six states

A goal is **accepted**, then **executing**, and ends **succeeded**, **aborted** (the server gave up: the arm hit a limit) or **canceled**. Between executing and canceled there is one more state, **canceling**: the client has asked and the server has not yet stopped.

```
accepted  -> executing, or canceling
executing -> canceling, succeeded, aborted
canceling -> canceled, succeeded, aborted
```

Each call says whether the move was legal. A goal that goes well passes through accepted, executing, succeeded. Anything else is refused, and the goal stays where it was: a goal cannot succeed before it executes, and a goal cancelled before it began cannot begin. See `E-BUS-0028`.

### Cancel is a request

Cancelling a goal does not make it canceled. It moves the goal to **canceling**, and only the server's confirmation, after the arm has actually stopped, makes it **canceled**. The gap is real: an arm decelerates for a second.

It also means a cancel can lose a race. The client cancels and the arm arrives in the same moment:

```
cancel requested, then the work finished before the cancel took effect: succeeded
```

The goal ends **succeeded**, and a client that assumed its cancel worked would be wrong about what the robot did. A cancel after the goal has finished is refused, and the state stays succeeded. See `E-BUS-0029`.

### Finished is final

A succeeded, canceled or aborted goal never changes again. The test made each of the three and then called every method on it: execute, cancel, succeed, abort, confirm: **0 changed**. A result that can be overwritten after it was reported is a result nobody can trust. See `E-BUS-0028`.

### Feedback has a time

Progress is only meaningful while the goal runs. Feedback to a goal that is accepted but not started, canceling or already finished is dropped, and counted:

```
feedback kept 2, dropped 3
```

A server that went on reporting 0.7 progress for a goal that was cancelled would have a client display a move that was no longer happening. See `E-BUS-0029`.

### A second goal

One arm cannot do two things. When a goal arrives and another is running, the server's rule decides:

| rule | second goal | first goal |
|---|---|---|
| reject new | refused (-1) | keeps executing |
| preempt old | accepted | canceling |

Under `reject_new`, the third goal, sent after the first had succeeded, was accepted with the next number. Under `preempt_old`, the first was asked to cancel, so it is **canceling** and not yet canceled: the second goal was accepted at once, while the arm is still stopping. A server has to decide whether the second goal waits for the confirmation. See `E-BUS-0030`.

## Build It

`request_cancel`, `succeed`, `abort`, `feedback` and `send_goal` in `exercise/solution.hpp`. `execute`, `confirm_canceled`, the state and the logs are given.

```
rcpp verify 17-10
```

Nothing here is timed.

## Use It

`rc/bus/action.hpp` joins `rc::bus`.

```cpp
ActionServer server(WhenBusy::reject_new);
const int id = server.send_goal();
if (id >= 0) { server.goal(id)->execute(); server.goal(id)->feedback(0.25); }
```

**What this does not do.** It is the state machine and not the messages: goal, feedback and result are three topics and two services in ROS 2, with the QoS of lesson 17-07 and the call semantics of lesson 17-09 under them. It keeps one goal active, where an action server may accept several. It does not say how long a cancel takes, which is the robot's. And the result is a state without data: the arm's final pose goes in the result message.

## What Breaks First

- **A goal that can be moved to any state.** A finished goal reopened, or a goal that succeeds before it starts. See `E-BUS-0028`.
- **A cancel taken for a stop.** The arm is still moving, or it finished. See `E-BUS-0029`.
- **A new goal with no rule for the old one.** Two moves at once, or a refused goal nobody knows about. See `E-BUS-0030`.

## Ship It

`rc/bus/action.hpp` holds `GoalState`, `is_terminal`, `GoalHandle`, `WhenBusy` and `ActionServer`.

A long running task can now be started, watched, stopped and trusted about how it ended.
