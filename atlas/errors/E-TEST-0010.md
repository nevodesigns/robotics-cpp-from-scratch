id: E-TEST-0010
title: A sleep where a wait belonged
match: waiting for a thing, not for a length of time
platforms: linux, windows
teaches: 05-05-the-test-that-fails-sometimes
---

## Symptom

A test starts a thread, sleeps for fifty milliseconds, and checks the result. It
passes everywhere except the build server, where it fails perhaps one time in
twenty. The sleep is raised to two hundred milliseconds. It passes, and the
suite is now four times slower for every run, including the runs that did not
need it.

Or the other version: a test hangs forever waiting for something that will never
happen, and the whole suite has to be killed.

## Cause

A fixed sleep is a guess about how long something else will take, and it is
wrong in both directions at once.

**Too short** and the test fails whenever the machine is busy, which is exactly
when a build server runs it. A machine four times slower needs a sleep four
times longer, and there is no number that is both.

**Too long** and every run pays for it whether or not it needed to. A suite full
of conservative sleeps takes minutes to say what it could have said in seconds,
and the slowness is charged to every developer on every commit.

There is no correct constant, because the quantity being guessed at is not a
property of the code.

## Fix

Wait for the thing, with a deadline.

```cpp
const bool ready = rc::test::wait_until([&] { return finished.load(); }, 5.0);
RC_CHECK(ready);
```

Measured on a worker doing a few hundred thousand additions, this returned in
**0.55 ms** against a deadline of five seconds. The common case costs whatever
the work actually cost, and the deadline exists only so that a condition which
never becomes true **fails the test** rather than hanging the suite.

That is the whole trade: a generous deadline costs nothing when things work,
because it is never reached, and a fixed sleep costs its whole length every
time.

Four details.

**Make the deadline generous.** It is not a performance assertion. Five seconds
for something that takes half a millisecond is fine, and it is what makes the
test survive a loaded machine.

**Check the condition before the deadline**, so something already true costs
nothing at all.

**Poll, do not spin.** A short sleep between looks keeps a waiting test from
taking a core away from the work it is waiting for.

**Assert that it did not time out.** `wait_until` returning false is the
interesting outcome and it must be checked, or the test carries on and fails
somewhere less informative.

The same shape applies outside tests: a robot waiting for a device to answer
wants a deadline and a decision, not a sleep, which is what `E-SENSE-0012` is
about from the other direction.
