# The First Tool: Asking, Checking, Driving, Drawing and Saying How It Ended

> Phase 00 gave you pieces: a step, a picture, an exit status, a checked number. This lesson joins them
> into one program, and the joining has its own ways of going wrong.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-07, 00-09, 00-10, 00-13

## The Problem

Each lesson so far made one thing work. A program somebody else runs is all of them at once: it is told what to do, it refuses nonsense, it does the work, it shows the result and it says
whether it succeeded, and the person on the other end sees only what comes out of it.

The failures that remain are in the joins. Output written before an error was found. An error message that says nothing. A picture that goes blank for the simplest case. None of the pieces is wrong on its own.

## The Concept

### Take the streams as parameters

A tool that reads `std::cin` and writes `std::cout` cannot be tested without a person. `run_drive` takes an input stream, an output stream and an error stream, and the program's `main` hands it the real
ones. The test hands it text and reads back what it wrote.

```cpp
int run_drive(std::istream& in, std::ostream& out, std::ostream& err);
```

It returns the status that `main` returns: 0 for success, 2 (misused) for bad input, as in lesson 00-09.

### Check everything, then act

The tool asks for three numbers: left speed from 0 to 2, right speed from 0 to 2, duration from 0.1 to 60 seconds. All three are read and checked before anything is driven or drawn.

Typing `fast` for the right speed gave status **2**, a message on the error stream and **0 characters** on the output. Output written before a problem is found cannot be taken back, and a script reading it
cannot tell a result from the start of one. See `E-CPP-0083`.

### Say which value and what was wrong

The checker from 00-10 knows which of four things happened. The message passes it on, with the allowed range:

```
right speed: not a number (allowed 0 to 2)
right speed: out of range (allowed 0 to 2)
duration: no input (allowed 0.1 to 60)
```

"Invalid input" would be a message the person could do nothing with. See `E-CPP-0084`.

### The same pieces, joined

With wheel speeds 0.75 and 1.25 for 1.5707963 seconds, the tool drives with the midpoint step of 00-13, scales a picture of 40 columns by 12 rows with the functions of 00-07, draws `*` at every pose, `S` at the start and `E` at the end, and
finishes with a line saying where it ended:

```
                      E
                      *
                     **
                     *
                    **
                   **
                  **
                ***
              ***
           ****
       *****
S*******
end x=1.000 y=1.000 heading=90.0
```

That is the picture the suite printed. The start is at the bottom left and the end at the top right, because y grows upward and the picture flips it. The end is in column 22 of 12 rows, so a quarter circle comes out about as wide as it is tall, with a character being about
twice as tall as it is wide.

### The simplest case

A drive at speed 1 and 1 for 2 seconds is a straight line along x, with no height at all. A scale that divides by the height divides by zero. `scale_to_fit` treats an axis with no extent on its own, and the suite found exactly **1** row of the picture
with marks in it, holding `S` and `E`, and the end line `end x=2.000 y=0.000 heading=0.0`. See `E-PLOT-0006`.

## Build It

`run_drive` in `exercise/solution.hpp`. The comments list the five parts and the earlier lesson each comes from.

```
rcpp verify 00-14
```

## Use It

`rc/core/drive_tool.hpp` joins `rc::core`. A `main` for it is four lines:

```cpp
int main() { return rc::core::run_drive(std::cin, std::cout, std::cerr); }
```

**What this does not do.** It asks in a fixed order and does not take its numbers from the command line (lesson 01-13 does). It drives at constant wheel speeds, and only one leg. The picture has a fixed size and no axes, so it shows the shape and not the scale; the
end line gives the numbers. And it does not repeat a question, since a tool that is fed from a script should fail and say why, where a person at a terminal may prefer to be asked again.

## What Breaks First

- **Output before the check is complete.** Half an answer. See `E-CPP-0083`.
- **A message that names nothing.** The person cannot act on it. See `E-CPP-0084`.
- **A path with no height.** The picture goes blank. See `E-PLOT-0006`.

## Ship It

`rc/core/drive_tool.hpp` holds `run_drive`.

That is phase 00: a robot that moves, a way to see it, a way to be wrong and find out, and a program that someone else can run and read.
