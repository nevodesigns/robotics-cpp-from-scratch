# A Power Cut at Every Step: Two Slots and a Commit, and Why Overwriting the Running Release Bricks the Robot

> A robot in a field loses power in the middle of an update. That is not a rare event. It is a certainty, given enough robots and enough updates.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated disk)
**Prerequisites:** 20-04, 19-11, 19-03, 03-03

## The Problem

The bundle of lesson 20-04 is on the robot, and checked. It has to become **the release the robot boots**. The obvious way is to copy it over the old one. That takes time (writing a few hundred megabytes to flash is seconds to minutes), and for all of that time the old release is **half old and half new**. If the battery dies, the cable comes out or the operating system crashes during that time, the robot has neither release, and it will not boot. In the trade this is **bricking**, and it is called that because the robot is then good for little else.

The same question was asked in lesson 19-03: what is the state when nothing is in charge? Here, the answer has to be that **the robot boots something**, whatever step the power failed at. The way to be sure is to write the install as a list of steps and **cut the power at every one**.

## The Concept

### A disk and its steps

A `Disk` has two **slots**, each holding a whole release as chunks with the hash that was stored for it and its version, and a **boot pointer** that says which to try first. A slot is **valid** if its stored hash is the hash of what it holds. `boot` takes the slot the pointer names if it is valid, otherwise the other one, otherwise nothing: `ok` false, and the robot reports it and does not guess. A pointer that is out of range is read as 0, and an empty slot is not valid, even though a hash of 0 might equal the hash of nothing.

An install is a **list of steps**, each a write that the disk does completely or not at all: write a chunk, write the hash and version, move the pointer. A power cut falls **between** steps, and never inside one. The last claim is an assumption about the hardware (a single aligned write to flash is atomic, and a pointer that is one word is), and it is the assumption every real scheme starts from, so it is stated here and not hidden.

### Overwriting in place

`plan_in_place` writes the new chunks into the slot that is running, one by one, and then the hash. Eight chunks, nine steps. Cut the power after each possible number of steps (0 to 9, ten cuts):

| installer | steps | boot release 1 | boot release 2 | bricked |
|---|---|---|---|---|
| overwrite in place | 9 | 1 | 1 | **8** |
| two slots and a commit | 10 | 10 | 1 | **0** |

In the first, any cut after the first chunk and before the hash leaves the slot holding chunks of both releases and the **old** hash: not valid, and there is no other slot. **8 of the 10** moments the power could fail leave a brick. See `E-DEPLOY-0013`.

### Two slots, and the commit last

`plan_ab` writes the slot that is **not running**: the chunks, then that slot's hash and version (the **commit**, after which the new release is complete and checked), and **last of all** the boot pointer. Ten steps. Nothing before the last changes what the robot boots, and the last is one write. After nine of the ten steps the new release is complete and valid in slot 1 and **the robot still boots release 1**; after the tenth it boots release 2, and **release 1 is still in slot 0**, untouched, for a way back. Ten of the eleven cuts boot release 1 and one boots release 2. None leaves nothing. See `E-DEPLOY-0014`.

The order is the whole design. Switch the pointer before the commit and a cut leaves the pointer on a slot that is half written (the robot then falls back, but only because the boot logic is careful); write the hash before the chunks and a cut leaves a slot that claims to be complete and is not.

### An install that was cut can be run again

An install cut at each of 11 points, and then run again from the start: **11 of 11** ended on release 2. The plan is chosen from what the robot **boots now**, not from where the last attempt got to, so it needs no memory of the failed attempt, and a robot that lost power in the middle of an update tries again when it is next powered, with the same code. An installer that has to know where it stopped is an installer that has a bug in the code that works out where it stopped.

### Release after release, and a bad image

Releases 2, 3, 4 and 5 used the slots 1, 0, 1, 0: they alternate, and the release before the current one is always in the other slot, valid. If a chunk of release 2 is damaged **after** the switch (flash that wore out, a bit that flipped), the boot finds the slot invalid and boots **release 1 from slot 0**. A robot that is healing itself, in a case that nobody planned for, because the fallback is the design. With release 1 damaged too, **nothing boots**, and that is reported as it is. A robot with nothing booting is installed from slot 0, from a recovery path. See `E-DEPLOY-0015`.

## Build It

`slot_valid`, `boot`, `apply`, `plan_in_place` and `plan_ab` in `exercise/solution.hpp`. The disk types and `image_hash` are given.

```
rcpp verify 20-05
```

## Use It

`rc/deploy/slots.hpp` joins `rc::deploy`.

```cpp
const auto steps = rc::deploy::plan_ab(disk, new_image_chunks, "1.4.3");
for (const auto& s : steps) apply_to_real_flash(s);           // each one a write the hardware does whole
```

and in a test, the loop of this lesson: apply the first `k` steps for every `k`, then `boot`.

**What this does not do.** The hardware assumption above is the one that matters, and it is false for some flash (a torn write inside a page, a controller that reorders writes, a file system that says a write is done before it is). The answer is `fsync`, barriers, and a hardware that is known to have atomic writes: **test it by pulling the real power**, hundreds of times, with the real disk. It does not check the **new** release works: a slot can be valid, bootable and broken, and the robot has then switched to it. That is the next lesson. It keeps two whole releases, which costs the space of twice the release, and a robot with little flash cannot afford it. It takes the hash on trust: a valid slot is one that matches the hash that was stored with it, and the hash should be the one that the signed manifest of lesson 20-04 carries. And it does not do anything for the **configuration and data** that the robot keeps, which a new release may need to migrate and an old one may not understand.

## What Breaks First

- **Overwriting the release that is running.** See `E-DEPLOY-0013`.
- **A commit that is not last.** See `E-DEPLOY-0014`.
- **A robot with nowhere to fall back to.** See `E-DEPLOY-0015`.

## Ship It

`rc/deploy/slots.hpp` holds `Slot`, `Disk`, `image_hash`, `slot_valid`, `Boot`, `boot`, `StepKind`, `WriteStep`, `apply`, `plan_in_place` and `plan_ab`.
