id: E-FIELD-0003
title: An arm's targets given to a leg
match: set_targets returns false for a command of the wrong size
match: a joint command with the wrong number of joints is half applied
match: joint buses move at their rate, refuse a wrong sized command, and keep arm and leg apart
platforms: linux, windows
teaches: 22-01-a-robot-as-an-interface
---

## Symptom

After a command built for another robot, some joints move and some do not. Code from another robot was reused with a different joint count.

## Cause

A command with the wrong number of joints was applied as far as it went. Half a command is worse than none: the robot is in a pose nobody asked for.

A command applied element by element up to its own length moves the joints it covers and leaves the rest where they were. The robot ends in a mixture of the old pose and the new one, which may not be safe.

## Fix

Check the size, refuse the whole command, and use separate types for separate robots. JointBus refuses it as a whole.

```cpp
if (targets.size() != target_.size()) return false;   // nothing applied
```
