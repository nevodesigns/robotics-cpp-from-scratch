id: E-BUILD-0001
title: Command run from the wrong directory
match: No such file or directory
match: could not load cache
platforms: linux, windows
teaches: 00-01-your-first-program
---

## Symptom

A command from a lesson reports that a file or directory does not exist, even
though the path in the lesson is clearly correct.

## Cause

Every command in this curriculum is written as a path from the repository root,
which is the folder containing CMakeLists.txt and phases. Running it from inside
a lesson directory makes those paths point at nothing.

## Fix

Change to the repository root and run the command again. To check where you are,
list the directory and look for CMakeLists.txt and phases side by side. rcpp
finds the root by itself, so rcpp verify works from anywhere inside the
repository.
