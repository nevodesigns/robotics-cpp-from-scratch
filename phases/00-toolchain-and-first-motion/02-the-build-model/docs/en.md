# The Build Model: Declaration, Definition, Linker

> A promise and a fact are different things, and the linker is what checks that every promise was kept.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-01

## The Problem

Your first program lived in one file. Real robot code does not. A drive
controller, a battery monitor, and a safety watchdog each live in their own
files, written by different people, and they still have to call each other.

The moment you split code across files you meet the error that stops more
beginners than any other in this language:

```
undefined reference to `celsius_from_raw(int)'
```

The compiler was perfectly happy. Something after the compiler was not. Until
you know what that something is, this error is unfixable magic. After this
lesson it is a five second fix, permanently.

## The Concept

Building a C++ program is two steps, not one, and they fail for different
reasons.

**Step one is compiling.** The compiler reads one `.cpp` file at a time, with no
knowledge of any other file. It checks that your text is valid C++ and produces
an object file of machine instructions. Each `.cpp` file is called a
**translation unit**, and the compiler sees exactly one at a time.

**Step two is linking.** The linker takes all those object files and stitches
them into one program. Its job is to connect every call to the actual code being
called.

That split explains everything. Consider:

```cpp
double celsius_from_raw(int raw);          // a declaration: a promise
```

```cpp
double celsius_from_raw(int raw) {         // a definition: the fact
  return raw * 0.0625;
}
```

The **declaration** tells the compiler "a function with this name and this shape
exists somewhere". That is enough for the compiler to check your call and move
on. The **definition** is the body, the real code.

If you declare and call but never define, the compiler is satisfied and the
linker is not. It reaches the end with a promise nobody kept, and reports an
undefined reference.

### Why headers exist

A header file is a bundle of declarations you paste into any file that needs
them, using `#include`. That is literally what `#include` does: it copies the
text of that file in at that point, before the compiler sees anything.

The rule that follows:

- Declarations go in the header, so many files can share them.
- Definitions go in one `.cpp` file, so the linker finds exactly one.

Define the same function in two `.cpp` files and the linker complains about a
duplicate symbol instead. Both errors come from the same rule seen from opposite
sides: exactly one definition, no more and no fewer.

### The exception every lesson file uses

Look again at `solution.hpp` from lesson 00-01. It is a header, and it holds a
whole function body. That looks like it breaks the rule above, and it does not,
because of the word at the front:

```cpp
inline int battery_percent(int millivolts) { ... }
```

`inline` changes what the linker is told to expect. Instead of **exactly one**
definition, it allows the same definition to appear in **every** file that
includes the header, and the linker keeps one. Without the word, a header holding
a body that two `.cpp` files include gives you the duplicate symbol error from the
paragraph above, and that is `E-LNK-0002`.

The lessons are written this way for a plain reason: the same test file is built
against your version and against the reference just by changing which folder it
includes from, and that is easiest when each version is a single header. It is a
choice about this curriculum and not a rule about C++. A project of any size
puts the declaration in a header and the body in a `.cpp`, which is exactly the
shape of this lesson's exercise.

What `inline` costs and guarantees when it is used on purpose, including the
trap where `static` looks like the same thing and is not, is lesson 04-04.
For now: when you write a function body in a header, start it with `inline`.

## Build It

`exercise/solution.hpp` declares two functions. `exercise/solution.cpp` defines
one of them and leaves the other missing on purpose.

Run this first, before changing anything:

```
rcpp verify 00-02
```

It will not even build. Read the error. Then paste it into:

```
rcpp explain
```

Now write the missing definition in `exercise/solution.cpp`. The functions:

- `celsius_from_raw(int raw)` converts a raw sensor count to degrees Celsius.
  Each count is 0.0625 degrees, so 320 counts is 20.0 degrees.
- `is_overheating(int raw)` answers whether the temperature is above 80 degrees.
  Write it in terms of `celsius_from_raw`, not by repeating the arithmetic.

## Use It

Every build system you will ever meet is automating these same two steps. CMake,
which lesson 04-01 introduces, decides which files get compiled and which object
files get linked together. When a real project fails to link, the question is
always the same one you just answered: which definition is missing, and which
file was supposed to hold it.

## What Breaks First

- **Undefined reference to a function you clearly wrote.** You declared it in the
  header and defined it nowhere, or you defined it with a slightly different
  signature so the linker sees two unrelated names. See `E-LNK-0001`.
- **Multiple definition of the same function.** You put a function body in a
  header that more than one `.cpp` includes. See `E-LNK-0002`.
- **The compiler reports an error inside a header you did not touch.** The real
  mistake is usually a missing semicolon at the end of the previous include. See
  `E-CPP-0003`.

## Ship It

Splitting an interface from its implementation is the habit that makes code
reusable at all. Every module in `librc` is shaped this way, and now you know
exactly which half the linker is looking for when it complains.
