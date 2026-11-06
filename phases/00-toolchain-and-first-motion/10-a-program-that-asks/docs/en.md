# A Program That Asks: Reading a Number a Person Typed, and the Four Ways It Goes Wrong

> A number in your program is a number. A number a person typed is text that might be one, and the obvious
> way to read it accepts things nobody meant.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-01, 00-04

## The Problem

Your programs so far have known every number they use, because you typed it into the source. A real program is told: a speed limit, a
distance, a target angle. Somebody types it.

People type 12abc. They type 3,5 and mean three and a half. They type nothing and press enter, they press the wrong key, and they type 400 for a speed limit that
tops out at 100. A script feeds the program a file that ends early.

The obvious way to read a number is one line, and it handles the first of these by accident and each of the others wrongly or not at all.

## The Concept

### What the one line does

`in >> value` reads as much of the text as looks like a number and stops. If that was nothing, it fails. If it was something, it succeeds and leaves the rest where it was.

The suite prints what it takes next to what `read_number` says:

| typed | taken by `>>` | read_number |
|---|---|---|
| 12abc | 12 | not a number |
| 3,5 | 3 | not a number |
| 7 8 | 7 | not a number |
| 9.5.1 | 9.5 | not a number |

Every one of those is a success to the one line. 3,5 is a speed of three, and nothing anywhere said so. See `E-CPP-0074`.

### Four ways it goes wrong

1. **Nothing there.** The input is empty or has ended. That is its own answer, `no input`.
2. **Not a number.** Words, or a number with something stuck to it.
3. **A number, out of range.** 400 against a limit of 100, or minus one for a distance. Valid as a number, no use as an answer, and said differently, with the limits themselves allowed.
4. **A number, fine.** Spaces around it are fine.

`read_number` returns an `Entry`: whether it worked, the value, and a `problem` that a person can read. A program that only says "invalid input" for all of them makes
the person guess which.

### A failed stream stays failed

After a read fails, the stream is in a failed state and every later read does nothing. The text that caused the failure is still in the stream as well. Two lines clean up, in this order:

```cpp
in.clear();                                                        // the failed state
in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');      // the bad line
```

Measured: `abc` followed by three sevens, ten reads with `>>` and no clean up, **0 succeeded**. A loop that asks until it is satisfied would spin at full speed for ever. See `E-CPP-0075`.

### Asking must end

`ask` calls `read_number` up to a number of times and stops at the first good answer. It also stops when the input has ended, because nobody is going to answer: asking an empty input up to 1000 times used **1 attempt**.
A program run by a script with its input gone is the case where a loop with no bound hangs, and it works fine when you test it by hand. See `E-CPP-0076`.

## Build It

`read_number` and `ask` in `exercise/solution.hpp`. The types are given, and the comments list the steps.

```
rcpp verify 00-10
```

The tests feed the functions from text in memory and not the keyboard, so they can run in a script and finish. That is the same trick you will use to test anything that asks.

## Use It

`rc/core/ask.hpp` joins `rc::core`.

```cpp
const Asked a = ask(std::cin, 0.0, 100.0, 3);
if (!a.entry.ok) { std::cerr << a.entry.problem << "\n"; return 1; }
```

**What this does not do.** It reads one number per line, so two numbers on a line are refused. It does not read text, which needs `std::string` (lesson 01-05), or say what was expected beyond the range. A real tool prints the range with the complaint: "speed must be between 0 and 100".

## What Breaks First

- **`>>` alone.** It accepts 12abc as 12. See `E-CPP-0074`.
- **A stream that was not cleaned up.** Every later read fails. See `E-CPP-0075`.
- **A loop with no end for input that has one.** See `E-CPP-0076`.

## Ship It

`rc/core/ask.hpp` holds `Entry`, `read_number`, `Asked` and `ask`.

Lesson 00-09 said what a program tells the outside world. This is the other direction, what the outside world tells the program, and the answer is to believe none of it until it has been checked.
