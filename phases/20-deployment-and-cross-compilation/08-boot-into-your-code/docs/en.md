# Boot Into Your Code: Service Units From a Spec, the Order They Start In, and a Crash Loop With and Without a Back-Off

> A robot that boots into your code has handed the decisions about when it starts, in what order and how often to restart to a file you did not think about.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-07, 17-20, 19-02, 03-03

## The Problem

The release is on the robot, checked and on trial. Something has to **start** it, and start the six programs that make up the robot in an order that works, and start them again when one dies. On a Linux robot that is the init system, usually systemd, configured by **unit files**. A unit file is a small text file written once, copied from the last project, and it is the place where the robot's behaviour at 3 am is decided: what happens when the camera driver crashes, in which order the nodes of phase 17 come up, and how many times a broken program is restarted.

The supervisor of lesson 17-20 does this inside the program. The init system does it from outside, and has to, since it is what starts the supervisor, and is what restarts it. Neither is optional.

## The Concept

### A unit file from a spec

`render_unit` writes one from a `Service`: where its program is, which services it comes **after**, which it **requires**, whether and how fast to restart, the limit on starts, an optional watchdog and a memory limit.

```
[Unit]
Description=perception
After=drivers
Requires=drivers
StartLimitIntervalSec=10
StartLimitBurst=5

[Service]
ExecStart=/opt/robot/bin/perception
Restart=on-failure
RestartSec=3
WatchdogSec=10
MemoryMax=512M

[Install]
WantedBy=multi-user.target
```

One setting to a line, in a fixed order, and `After`, `Requires`, `WatchdogSec` and `MemoryMax` left out when there is nothing to say. The point of generating it from a spec is the point of lesson 20-02: **one source**, which can be checked.

### The order they start in

`start_order` gives an order in which each service comes after everything it names in `After`, and where two could go first, the one with the smaller name, so that it is the same every time. Five services (`drivers`, `perception` after drivers, `planner` after perception, `logger`, `bridge` after planner and logger) started as `drivers -> logger -> perception -> planner -> bridge`. A cycle of three services started **nothing**; a service that names itself started nothing; a name that is not a service is ignored here and caught by the check below.

### What is wrong, named before it is installed

`validate_services` on a set of services that had been written in a hurry gave **6 problems**:

- `perception: exec is not an absolute path`
- `planner: requires perception but is not ordered after it`
- `logger: after unknown service: ghost`
- `logger: requires unknown service: phantom`
- `logger: restart storm: restart with no wait`
- `drivers: duplicate service name`

The second is the one that is found at the first boot on a faster board: `Requires` says the planner cannot run **without** perception, and says nothing about **order**. They start together, and the planner starts, finds nothing, and fails, on half the boots. The remedy is a line in `After`. A cycle, `a` after `b` after `a`, is one problem on the first name that cannot start: `ordering cycle`. See `E-DEPLOY-0022`.

### A crash loop

A service that crashes 0.2 s after every start, watched for 60 s. Three ways of restarting it:

| policy | starts in 60 s | |
|---|---|---|
| restarted after 0.1 s, no limit | **201** | forever |
| waits 1, 2, 4, 8, 16, 30 s, limit 5 in 10 s | **6** | still being tried |
| restarted after 0.1 s, limit 5 in 10 s | 5 | **gave up at 1.5 s** |

The first is the default of a lot of hand written units: restart at once, always. At about three starts a second a broken program fills the log, keeps a core busy, and starts a driver that opens a device and fails to close it. The third is what the system does when a **start limit** is set and the restart is quick: five starts in 1.5 s and it **gives up**, leaving the service failed, and the robot without that program until somebody notices. The second is the answer: wait longer after each crash (1, 2, 4, 8, 16, then 30 s), so a persistent fault costs six starts a minute and not two hundred, and the limit of five in ten seconds is never reached since the starts are far enough apart. A restart policy is a choice between **spamming** and **giving up**, and a back-off is how to do neither. See `E-DEPLOY-0023`.

### The back-off that remembers too much

A service that runs for 100 s between crashes, watched for 1000 s. With a back-off that **never resets** the waits grow to the cap of 30 s and it started **9** times; with one that **resets after a run of 30 s**, it started **10** times, each with a wait of 1 s after a 100 s run. The difference looks small and the cause is not: a service that has been healthy for a day and then crashes once should be restarted at once, and not wait 30 s because it crashed a lot in the first minute after boot. The back-off is a response to a **loop**, and a long run ends the loop. See `E-DEPLOY-0024`.

## Build It

`render_unit`, `start_order`, `validate_services` and `simulate_restarts` in `exercise/solution.hpp`. The types and the ordering helper are given.

```
rcpp verify 20-08
```

## Use It

`rc/deploy/services.hpp` joins `rc::deploy`.

```cpp
const auto problems = rc::deploy::validate_services(services);
if (!problems.empty()) return fail(problems);
for (const auto& s : services) write_file("/etc/systemd/system/" + s.name + ".service", rc::deploy::render_unit(s));
```

**What this does not do.** It writes the common fields of one init system, and a real unit has dozens more (sandboxing, users, capabilities, devices, sockets, `OnFailure`). It does not run `systemd`, and **systemd's own rules differ in details** (its start limit and its restart delay interact with each other, and a unit that is given up on stays failed until reset): the real check is `systemd-analyze verify` on the target. It does not model a service that **hangs**, which is a watchdog's job, and a watchdog (`WatchdogSec`) is only as good as the program that is supposed to report to it: a program that reports from a thread that does nothing but report is a program with no watchdog. And it does not say what to do when the service is **given up on**. A robot with a failed perception service and a working drive service is the worst combination, a robot that moves and cannot see, and the answer is in phase 19: **a failed safety-relevant service is a stop**, and it is for the supervisor of 19-10 to see it.

## What Breaks First

- **A requirement that is not an ordering.** See `E-DEPLOY-0022`.
- **A restart with no wait, or a limit that gives up in a second.** See `E-DEPLOY-0023`.
- **A back-off that never forgets.** See `E-DEPLOY-0024`.

## Ship It

`rc/deploy/services.hpp` holds `Restart`, `Service`, `UnitProblem`, `render_unit`, `start_order`, `validate_services`, `RestartPolicy`, `RestartRun` and `simulate_restarts`.
