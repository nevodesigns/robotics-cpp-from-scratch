# Your First Program

> A program is a machine you build out of text, and a compiler is the machine that builds it.

**Type:** Build
**Time:** about 60 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, this runs anywhere
**Prerequisites:** none, this is the first lesson

## The Problem

A robot has a battery. The battery reports its voltage, not its charge, because
voltage is the only thing the hardware can actually measure. A lithium cell
reads about 4200 millivolts when it is full and about 3000 millivolts when it is
empty, and somewhere in the middle it reads 3600.

Nobody wants to look at millivolts. The operator wants a percentage. So
somewhere in every robot there is a small piece of code that turns one number
into another number, and if that code is wrong the robot dies in a field while
the screen says sixty percent.

Your first program is that piece of code.

## The Concept

A computer does not understand the text you write. It understands numbered
instructions, and it will not read English. A **compiler** is a program that
reads your text and writes those instructions into a file you can run.

That gives you the loop you will repeat for the rest of this curriculum:

1. You write text into a file.
2. The compiler turns that text into a program, or refuses and tells you why.
3. You run the program, and it does exactly what you wrote, which is not always
   what you meant.

The unit of work in C++ is a **function**. A function has a name, takes some
values in, and hands one value back. Here is the whole shape of one:

```cpp
int add(int a, int b) {
  return a + b;
}
```

Read it right to left in pieces. `add` is the name. `(int a, int b)` says it
takes two whole numbers, and inside the function they are called `a` and `b`.
The leading `int` says the value it hands back is a whole number too. `return`
is the word that hands it back.

That is a complete idea and there is nothing hidden inside it. A program is many
of these, arranged so that the answer comes out at the end.

### Whole numbers and fractions are different things

C++ has separate types for whole numbers and for numbers with a fractional part.
`int` holds whole numbers like 42. `double` holds numbers like 42.7. This
distinction feels pedantic until it silently destroys your battery reading, which
is exactly what happens in lesson 03. For now, simply notice that they are not
the same word.

## Build It

Open `exercise/solution.hpp`. It contains one function with its body missing.
You are writing `battery_percent`, which converts a battery reading in
millivolts into a percentage from 0 to 100.

The rules, exactly:

- 3000 millivolts or less is 0 percent. A reading below the empty point is still
  0 percent, never a negative number.
- 4200 millivolts or more is 100 percent. Never more than 100.
- Between those, the percentage rises evenly. A reading of 3600 is exactly
  halfway between 3000 and 4200, so it is 50 percent.

Run this from the repository root:

```
rcpp verify 00-01
```

It will fail at first. That is the design. Read the first failing check, it names
the exact number it expected and the number it got.

## Use It

Real firmware does not use a straight line like this one, because a lithium cell
does not discharge evenly. Production code uses a lookup table of measured
voltage points, or tracks current over time to count charge in and out, which is
called coulomb counting.

The straight line version is still the right first answer. It is honest about
what it knows, it cannot crash, and it is five lines long. A great deal of
working robot code is exactly this: a small, obviously correct function that
somebody trusted enough to leave alone.

## What Breaks First

These three account for nearly every failure on this lesson.

- **You forgot the semicolon at the end of a statement.** C++ needs one after
  every statement, and the error it prints usually points at the line *after* the
  real problem. See `E-CPP-0001`.
- **You wrote `=` where you meant `==`.** One assigns a value, two compares
  values. See `E-CPP-0002`.
- **You ran the command from the wrong directory.** Every command in this
  curriculum runs from the repository root, the folder holding `CMakeLists.txt`.
  See `E-BUILD-0001`.

When something fails and the text means nothing to you, paste it into:

```
rcpp explain
```

## Ship It

The function you just wrote is the first entry in your own library. Later
lessons will move it into `librc`, alongside everything else you build, until
you own a robotics library that you wrote line by line and can explain to
anyone who asks.
