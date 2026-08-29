id: E-TEST-0001
title: Passes with the fake, fails on the hardware
match: expected .*reads
platforms: linux, windows
teaches: 05-02-fakes-and-replay
---

## Symptom

A driver is fully tested, every test is green, and it misbehaves the first time
it is connected to the real device.

## Cause

The fake is more cooperative than the device. It answers immediately, always
returns a value in range, never disappears halfway through a read, never returns
a partial frame, and never takes longer than expected.

That is not a flaw in the idea of a fake. It is a statement that the fake models
what you thought of, and the device does more than you thought of.

## Fix

Treat every difference discovered on hardware as a gap in the fake, and close
it. When a real sensor returns a value outside its documented range, teach the
fake to return one. When a real port delivers half a message, teach the fake to
deliver half a message.

Recording real sessions and replaying them shortens this loop considerably,
because a recording contains oddities nobody would have invented.

Keep one test that runs against real hardware, run rarely and deliberately, and
treat every failure in it as a fake that needs improving rather than as a reason
to distrust fakes. The point of the fake is that the failure is reproducible
afterwards.
