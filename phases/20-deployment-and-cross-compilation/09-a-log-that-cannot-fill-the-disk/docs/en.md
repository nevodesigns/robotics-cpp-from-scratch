# A Log That Cannot Fill the Disk: Rotation Under a Byte Budget, and a Guard Against One Error Said a Million Times

> The robot that ran for a year without a problem will die of a full disk, and the disk will be full of the one message that said so.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-08, 19-12, 03-03, 01-05

## The Problem

The robot's flash is small: a few gigabytes, shared by the operating system, the two releases of lesson 20-05, the maps, the configuration and the logs. A log that grows is a bomb on a timer, and it fails in the worst way. A full disk does not stop the program that is writing the log; it stops **everything else that writes**: the configuration save, the black box of lesson 19-12 when it is dumped, the update that wants to put the next release in the other slot, a process that wants a temporary file. The robot misbehaves in ways that look like ten different faults, and the cause is the message at the top of the log, repeated ten million times.

Two defences, and both are cheap: **a budget** that the log is never allowed to exceed, and a **guard** that stops one message being written a million times.

## The Concept

### A budget, and what it is made of

A `LogPolicy` has a file size (10 MB), a total (200 MB) and a number of files (20). `policy_valid` refuses a policy that is not usable (a file that holds nothing, no files, a total smaller than one file), and `budget` is the most the log can take up in the steady state: the smaller of the total and the number of files times the size of a file. Twenty files of 10 MB is exactly 200 MB; with at most 5 files the budget is **50 MB**, since the count is the tighter limit; with a total of 30 MB it is **30 MB**. The budget is the number to put in the disk plan.

### Oldest first, never the one being written

`LogStore` appends to the current file; when the next record would take it past the file size it is **closed** and a new one is started; and then the oldest closed files are deleted while there are too many or the total is over. The files that are left are always the **newest**, **consecutive** (nine records of 40 bytes, files of 100 bytes and a total of 300: file numbers 1 to 4 kept, 280 bytes, one file of 80 deleted, nothing missing in the middle). The deleted bytes and the kept bytes together are everything that was written. A record of nothing is ignored. See `E-DEPLOY-0025`.

The current file is **never** deleted, which has one consequence that should be stated: a record bigger than the whole budget is kept. A record of 9000 bytes against a budget of 5000 was held, and dropped when the next record closed its file. A log that refuses a large record loses the one thing somebody wanted (a stack trace, a dump), and the cost of keeping it is one record's worth, until the next.

### A day

Logging at 100 KB a second (ten records a second of 10 KB, plausible for a robot at debug level) for 24 hours:

| | written | |
|---|---|---|
| no limit | 8.6 GB | a 4 GB disk **full after 11.1 hours** |
| 10 MB files, 20 files, 200 MB | 8.6 GB | peak **199 MB** in at most 20 files, 8.44 GB deleted |

Without a limit the robot dies at 11 hours, in the middle of its first shift. With the budget the same day leaves **199 MB** and the last twenty files: about **35 minutes** of the log at that rate, which is the thing to know when choosing the budget: what you will have when you need it. A budget is a promise about the **past** you will keep, and it is short at a high rate. The record of the last half hour, from the black box, and a summary, are worth more than all of the rest. See `E-DEPLOY-0025`.

100000 records of random sizes from 1 to 900 bytes against a budget of 5000 bytes in files of 1000: **0** breaches, and the most it ever held was **5000**. The property is checked on every record and not at the end.

### One message, a million times

A camera that has lost its cable says `camera: no frame` on every frame. At 1000 times a second for 10 s, that is 10000 lines in ten seconds and 8.6 million in a day, from one fault, and every one is the same. `FloodGuard` allows the first **5** in a window of 10 s and **suppresses and counts** the rest: **5 written, 9995 suppressed**, and the count is kept so that `take_suppressed` hands it over once, for one line that says "repeated 9995 times". Another message is not held back (the lidar's timeout got through), a message after the window opens a new window, and a clock that goes back opens a new one and does not leave a message muted. The guard changes a log that grows with the number of faults into one that grows with the number of **different** faults. See `E-DEPLOY-0026`.

Rotation under a budget without a guard still loses: the one message fills the budget and **pushes out** everything that came before it, and the log of the hour before the fault, the part that explains it, is deleted by the fault. The two go together. See `E-DEPLOY-0027`.

## Build It

`policy_valid`, `budget`, `LogStore::append`, `FloodGuard::allow` and `take_suppressed` in `exercise/solution.hpp`. The rest is given.

```
rcpp verify 20-09
```

## Use It

`rc/deploy/logstore.hpp` joins `rc::deploy`.

```cpp
rc::deploy::LogStore store{rc::deploy::LogPolicy()};
rc::deploy::FloodGuard guard(5, 10.0);
if (guard.allow(message_id, robot_clock())) {
  write_line(message);
  store.append(message.size() + 1);
}
```

**What this does not do.** It models the **sizes**: it does not write a file, flush it, or make the deletion safe against a power cut (a log that is half deleted is a thing that lesson 20-05's method of ordering would help with). It counts bytes the program writes, and a file system has overheads (blocks, journals, a minimum size per file) that make the real use larger. It does not **compress**, which is the cheapest way to keep ten times more history, nor ship the log off the robot, which is the only way to keep it after the robot is gone. A log stored on the robot is read by the person who has the robot, and the worst moments are the ones when nobody does. And the guard identifies a message by an id the caller chooses: if the id includes a changing number (a timestamp, a counter), every message is new and the guard does nothing.

## What Breaks First

- **A log with no limit.** See `E-DEPLOY-0025`.
- **One message said a million times.** See `E-DEPLOY-0026`.
- **A budget that the flood pushes the history out of.** See `E-DEPLOY-0027`.

## Ship It

`rc/deploy/logstore.hpp` holds `LogPolicy`, `policy_valid`, `budget`, `LogStore`, `FloodGuard`.
