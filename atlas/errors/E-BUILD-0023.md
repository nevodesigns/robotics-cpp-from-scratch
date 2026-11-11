id: E-BUILD-0023
title: A GPL only module linked without anyone noticing
match: each way of getting it wrong is a finding of its own, with the module or the missing thing named
match: expected gpl.findings
platforms: linux, windows
teaches: 12-10-what-a-shipped-binary-owes
---

## Symptom

A program is shipped with a chart widget that someone added from the Qt add-ons, and later a letter says the whole program is under the GPL.

## Cause

A few Qt modules are offered under the GPL 3.0 only or a commercial licence. They are installed beside the others and used in the same way, and nothing in the build says which is which. Measured: a list with Core and Charts gave **1** finding, `gpl-only`, naming charts.

## Fix

Check the modules the program links against the licence of each before it is shipped:

```cpp
if (licence_of(name) == Licence::kGplOnly) a.findings.push_back(Finding{"gpl-only", ...});
```
