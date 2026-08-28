id: E-CMAKE-0002
title: No rule to make the target you asked for
match: ninja: error: unknown target
match: No rule to make target
platforms: linux, windows
teaches: 00-02-the-build-model
---

## Symptom

Building a named target fails immediately, reporting that the target is unknown.

## Cause

Either the name is wrong, or the target was never created for this configuration.
In this repository a lesson target only exists when the lesson claims the
toolchain being configured, and Qt lessons are skipped entirely when Qt is not
installed. Both cases are reported during configuration.

## Fix

Run rcpp list to see the lessons that exist, and read the configure output for
lines saying a lesson was skipped. Lesson target names follow the pattern
lesson_00_01_your_first_program_exercise. Use rcpp verify with the lesson id
instead, which works out the target name for you.
