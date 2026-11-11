id: E-BUILD-0025
title: A notice and an offer that were not shipped, and a licence that was guessed
match: everything wrong at once is every finding, in a fixed order, and a module spelt two ways is one finding
match: the modules of a control station are LGPL, four add-ons are GPL only, and a module that is not in the table is unknown and not guessed
platforms: linux, windows
teaches: 12-10-what-a-shipped-binary-owes
---

## Symptom

The program uses Qt properly and ships with no licence text, or the table of a script says LGPL for a module that nobody looked up.

## Cause

What the LGPL asks for is a notice that Qt is used, its licence text, and an offer of the source of the Qt that is shipped. A module that is not known has to be looked up, and a script that treats it as one of the others is wrong when it is not. Measured: with everything wrong, **6** findings in a fixed order; `Frobnicate`, `Quick3D` and `Multimedia` were **unknown**.

## Fix

Say what is missing, and say unknown for what is not known:

```cpp
if (!ships_licence_text) a.findings.push_back(Finding{"no-licence-text", ...});
return Licence::kUnknown;      // not LGPL by default
```
