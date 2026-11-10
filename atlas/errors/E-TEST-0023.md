id: E-TEST-0023
title: A test whose false alarm rate was never measured
match: three spreads fails about once in four hundred, and six spreads did not fail once in ten thousand
match: expected six to equal
platforms: linux, windows
teaches: 05-09-the-bound-you-picked
---

## Symptom

A suite of three thousand tests fails one time in a few runs, each time on a different test, and nobody can reproduce any of them. Each test on its own passes a thousand times in a row.

## Cause

A test with a false alarm rate of one in 400 looks perfect when run a few times, and a suite of 3000 of them has several failures in every run. A rate is only known by running the test on many seeds and counting.

Measured, over 10000 seeds: a bound of three spreads failed **26** times (about one in 400), and a bound of six spreads failed **0**. For six spreads the true rate is about one in five hundred million.

## Fix

Run the test over many seeds and count, with `count_failures`, before it goes into the suite, and give a statistical test a bound of about six spreads:

```cpp
const int failures = count_failures(test, 1, 10000);       // expect 0
```
