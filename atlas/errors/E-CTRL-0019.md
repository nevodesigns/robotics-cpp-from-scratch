id: E-CTRL-0019
title: A friction model that never lets the mass stop
match: a moving mass with no force on it stops, and then stays exactly stopped
match: a mass that is stuck stays stuck until the force passes the breakaway, and not before
platforms: linux, windows
teaches: 14-08-stuck-then-slipping
---

## Symptom

In simulation a mass that should come to rest vibrates about the stopping point, at the
step rate, with a tiny speed that never settles to zero. A controller tested on it looks
noisy, a data log shows a velocity of plus and minus a few micrometres a second, and a
check that the mass has stopped never passes.

## Cause

Friction was written as `sign(velocity) * coulomb`. At exactly zero velocity that is zero,
and in discrete time a mass crossing zero never lands on it: it is slightly positive, so
friction pushes it back, past zero, slightly negative, so friction pushes it back again.
It chatters across zero for ever, at the step rate, and the force that should hold it is
never applied.

Stiction is the other half of the same defect. Static friction holds a mass that is not
moving with any force up to a limit, and that is a statement about a state, stuck, and not
about a velocity.

## Fix

Make it a state. Stuck: the mass does not move while the applied force is no more than the
breakaway. Slipping: friction is the Coulomb force opposing the motion, and if one step would
carry the velocity through zero, the mass stops there and is stuck again.

```cpp
double next = velocity + acceleration * dt;
if (velocity * next < 0.0) next = 0.0;   // it would have gone through zero: it stops
```

The test checks the stopping time against the equations: slowing from 2 m/s under a Coulomb
force of 1 N and a drag of 0.5 N per metre a second takes `2 ln 2 = 1.386` seconds, and the
simulation agrees to within a hundredth. After that the velocity is exactly zero for five
thousand steps, not small. And the force that starts a mass is more than the force that keeps
it going: 1.2 N will not start it and will keep it moving at 0.4 metres a second.
