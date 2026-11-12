id: E-DEPLOY-0022
title: A requirement that is not an ordering
match: a set of services that would not start, or would start in the wrong order, is named in full before it is installed
match: services start in an order that respects what they wait for, the smaller name first when it does not matter, and a cycle starts nothing
platforms: linux, windows
teaches: 20-08-boot-into-your-code
---

## Symptom

The robot starts on the bench and on half the boots in the field, and the planner reports that perception is not there.

## Cause

Six problems were found in a hurried set of units, among them a service that requires another without being ordered after it, an unknown name, a relative exec and a restart with no wait. Five services started as drivers, logger, perception, planner, bridge; a cycle of three started nothing.

## Fix

Put the required service in After as well.

```cpp
service.after.push_back(required_name);
```
