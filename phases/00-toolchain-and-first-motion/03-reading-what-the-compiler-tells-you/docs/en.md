# Reading What the Compiler Tells You

> Six errors, four mistakes, and one of the fixes removes three of them. Knowing
> which one is a skill, and it is the only skill in this curriculum that every
> later lesson quietly assumes you already have.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-02

## The Problem

You changed one line and the screen filled with red. The last error mentions a
file inside the standard library that you have never opened. None of it looks
like anything you wrote.

The reasonable response, and the one almost everybody has, is to feel that the
whole thing is broken and to start deleting.

It is not broken and there is probably one mistake. This lesson is about getting
from that screen to that mistake, and it is a lesson rather than a footnote
because it is the difference between an hour and five minutes, several times a
day, for the rest of the time you write C++.

## The Concept

### An error message has a shape

```text
solution.hpp:32:26: error: expected ';' before 'if'
   32 |   if (value < 0) return 0
      |                          ^
      |                          ;
```

Five parts, and each is doing work:

- `solution.hpp` is the file.
- `32` is the line, and `26` is the column.
- `error:` says this stopped the build. A `warning:` did not.
- The message names what the compiler expected and what it found instead.
- The **caret** points at the exact column, and here the compiler has printed
  the character it wanted directly underneath.

Read the caret before the message. It is the most precise thing on the screen
and it is the part people skip.

### The count of errors is not the count of mistakes

The compiler reads your file once, from the top. When it meets something it
cannot make sense of, it guesses what you probably meant and carries on, so that
it can report more than one problem per build. That is a kindness with a cost:
when the guess is wrong, everything after it is reported against a
misunderstanding rather than against your code.

The exercise in this lesson has four mistakes. Measured:

| | GCC 11 | Clang 14 |
|---|---|---|
| errors reported | 6 | 6 |
| errors after fixing only the first | 3 | 3 |

One fix, three errors gone, and none of those three were touched.

So: **fix the first error, then rebuild before reading anything else.** Not the
first three. Not the easiest looking one, which is the natural instinct and is
usually wrong, because the easy looking ones near the bottom are the most likely
to be consequences of the hard looking one at the top.

### The lines that are not errors are often the answer

```text
error: 'sqrt' is not a member of 'std'
note: 'std::sqrt' is defined in header '<cmath>';
      did you forget to '#include <cmath>'?
```

The second line is the entire fix, and it is skipped constantly, because it does
not say `error` and it sits in the middle of a wall of text.

`note:` lines tell you which header you forgot, where a function was declared
and how many arguments it wanted, and which of several overloads was being
considered. When an error is confusing, the note under it is where to look next.

### Where a missing semicolon is reported

The old advice is that a semicolon error always points one line too far down.
Measured on GCC 11 and Clang 14, that is now mostly untrue:

| the mistake | GCC | Clang |
|---|---|---|
| `return 0` with no semicolon, before `}` | right line | right line |
| a struct member with no semicolon | right line | right line |
| a struct's closing brace with no semicolon | right line | right line |
| `int a = 1`, next line another declaration | **next line** | right line |

Three cases in four now name the line you want. The old rule survives in one
case and only on GCC, and the reason is worth understanding rather than
memorising: nothing is actually wrong until the next declaration begins, so that
is where the compiler notices.

The rule that always works: look at the caret, and if the line under it is fine,
the statement above it did not end.

### Compiler errors and linker errors are different animals

Lesson 00-02 separated compiling from linking. Their errors look different and
the difference tells you which half of the build failed:

| | compiler error | linker error |
|---|---|---|
| names | a file, a line, a column | a symbol, and no line at all |
| typical text | `expected ';' before` | `undefined reference to` |
| means | this file does not make sense | this file makes sense, and something it promised was never written |

A linker error with no line number is not a worse error. It is a different
question: not "what did I write wrongly" but "which definition is missing".

### When you are stuck

```
rcpp explain "expected ';' before 'if'"
```

The Failure Atlas holds catalogued failures with their causes and fixes, and
`rcpp explain` searches it. You can also pipe a whole build into it:

```
cmake --build build 2>&1 | rcpp explain
```

Every entry it holds came from somebody actually hitting the error.

## Build It

`exercise/solution.hpp` does not compile. That is the exercise.

There are four mistakes in it. Fix them one at a time, in the order the compiler
reports them, rebuilding after each:

```
rcpp verify 00-03
```

Watch the error count as you go. It will not fall by one each time, and which
fix removes which errors is the thing to notice.

Once it compiles, the tests still have to pass. Making the file compile by
deleting what it was doing is not a fix, and there is a test for each function
that says so.

## Use It

This is the loop for the rest of your life in this language: build, read the
first error, change one thing, build again.

The habit worth taking from it is the rebuild. Reading six errors and making six
changes means five of those changes were made against a screen that was already
out of date, and when the count goes up afterwards you have no way to tell which
change caused it.

## What Breaks First

- **A name that is not a member of std.** The header was never included, and
  one missing line produces an error at every place the header was used. See
  `E-CPP-0025`.
- **Expected a semicolon.** Look at the caret, then at the statement above it.
  See `E-CPP-0001`.
- **A wall of errors from one mistake.** Fix the first, rebuild, read again.
  See `E-CPP-0026`.

## Ship It

Nothing here graduates into a library, and that is the point: what you keep is
the ability to make a file compile by reading rather than by guessing. Every
lesson after this one assumes it, and the Failure Atlas exists because it is
learnable rather than innate.
