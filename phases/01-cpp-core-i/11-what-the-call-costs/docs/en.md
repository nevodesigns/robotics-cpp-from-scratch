# What the Call Costs: Passing, Returning, and the Copy You Did Not Write

> Passing a vector of 1024 doubles into a function copied all of it. Returning
> the same vector out of a function copied none of it. Most people expect the
> opposite.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-04, 01-05

## The Problem

You have written a great many functions by now and nobody has told you what
happens when one is called.

That has been fine, because the things you have passed around have been small.
It stops being fine the moment a function takes a buffer of sensor readings,
which is every function in phases 06 and 15. A reading pipeline that copies its
input at each stage does work nobody wrote and nobody can see, and the only
clue is that the arithmetic cannot account for the time.

This lesson answers it by measuring rather than explaining, and the instrument
is the address of the data.

## The Concept

### How to see a copy

A `std::vector` keeps its elements in a buffer somewhere else, and `data()`
reports where. So:

- if the callee sees the **same** address the caller had, no copy was made
- if it sees a **different** one, a whole new buffer was allocated and every
  element copied into it

That is the entire technique, it needs no instrumentation and no profiler, and
every number below comes from it.

### Passing

A vector of 1024 doubles, through three signatures:

| how it is passed | what arrives |
|---|---|
| `std::vector<double> samples` | **a new buffer** |
| `const std::vector<double>& samples` | the same buffer |
| `rc::span<const double> samples` | the same buffer |

And the cost is **per call, not once**. Passing the same vector by value through
two functions:

| | buffers allocated |
|---|---|
| by value | **2** |
| by const reference | 0 |

A chain of five reading functions allocates and copies five times. Each line
looks innocent on its own, which is why this is so common.

### Returning, where the instinct is wrong

Having seen that, the natural conclusion is that returning a container must be
worse, because the thing has to get out somehow. So people write this:

```cpp
void compute_path(const Map& map, std::vector<Pose>& out);
```

Measured, by recording where the local vector's buffer lived inside the
function and comparing against what the caller received:

> **the same buffer arrived.**

At `-O0` as well as `-O2`. Whether the compiler elided the local entirely or
moved it, the buffer was handed over rather than rebuilt. There was never a copy
to avoid.

So the out parameter bought nothing, and it cost:

- the result cannot be `const`
- it needs a declaration on a line of its own, before you know whether the call
  will even succeed
- a function whose answer is not its return value cannot be used in an
  expression
- a half filled output after an early return is a state the caller must now
  think about

**Return the thing.** `return local;` plainly, not `return std::move(local);`,
which is unnecessary and can prevent the elision it was meant to help.

The other thing people reach for is worse: returning a *reference* to the local.
The local is destroyed as the function returns, so the reference refers to freed
memory. Both compilers refuse it once warnings are errors, and this lesson keeps
one in `checks/` so you meet the error deliberately rather than at three in the
morning.

### Why a span and not a const reference

Both avoid the copy, so the choice looks like taste. It is not.

`const std::vector<double>&` demands that the caller **own a std::vector**. A
span asks only that the data be laid out in a row. Measured with the same
function called three ways:

```
a span took a vector, a std::array and a plain buffer, and copied
none of them. A const vector reference would have needed a vector
built from the last two.
```

That last point is the one that matters in robotics. A driver hands you a
pointer and a length. With a span, you call the function. With a const vector
reference, you construct a vector from the driver's buffer, which allocates and
copies, which is exactly the cost the signature was chosen to avoid.

So: **take a span for anything you only read.** Take a const reference when the
parameter genuinely is that container, because you need `capacity()` or will
pass it on to something that wants a vector. Take by value when you want your
own copy, and say so.

### Strings, where the usual advice needs a qualifier

"Pass strings by const reference" is advice about *long* strings, and whether a
string is long is a property of your standard library.

A short string keeps its characters **inside the string object itself**, so
copying it allocates nothing: it moves a handful of bytes within one object.
Measured, with `sizeof(std::string)` of 32:

| length | characters live | copying it allocates |
|---|---|---|
| 0 | inside the object | nothing |
| 1 | inside the object | nothing |
| 15 | inside the object | nothing |
| 16 | on the heap | a buffer |
| 64 | on the heap | a buffer |

That threshold is an implementation choice, not a language rule, which is why
the suite prints the table rather than asserting the number. Measure it on your
toolchain rather than quoting one.

A long string is a different matter. At 256 characters, by value arrives as a
new buffer and by const reference arrives as the same one.

The conclusion is not a rule about strings, it is a question to ask: is this
parameter a unit label or a log line? For anything you only read,
`std::string_view` is to strings what a span is to buffers, and it removes the
question instead of answering it per parameter.

## Build It

Implement `passed_as_span`, `passed_by_value_twice`, `built_and_returned` and
`stored_inside_the_object` in `exercise/solution.hpp`.

```
rcpp verify 01-11
```

The suite compares addresses across three parameter forms, counts the buffers a
two call chain allocates, checks what arrives when a container is returned,
calls one span function with three kinds of container, and sweeps string lengths
to find where the characters move out of the object. Two snippets in `checks/`
are compiled and expected to fail.

Before you run the returning test, write down what you expect. Most people
expect a thousand doubles to be copied on the way out.

## Use It

**Take `rc::span<const double>` for anything you only read.** This is the single
change that matters most, and from here on every function in this curriculum
that reads a buffer is written that way.

**Return results, do not fill out parameters.** Keep an out parameter only when
the real reason is avoiding an allocation in a control loop, and write that
reason down.

**Return a named local plainly.** No `std::move` on the way out.

**Never return a reference or pointer to a local.**

**Ask which kind of string a parameter is**, and prefer `std::string_view` when
you only read it.

**Measure before you optimise a signature.** Comparing two addresses is four
lines and settles it, and this whole class of change is frequently a rewrite
that moves no bytes at all.

## What Breaks First

- **A buffer copied at every call.** See `E-CPP-0037`.
- **An out parameter avoiding a copy that never happened.** See `E-CPP-0038`.
- **String advice applied without knowing which strings.** See `E-CPP-0039`.

## Ship It

This lesson ships no header. What it changes is every signature you write from
here on, and the rule is short enough to carry: **borrow what you read, return
what you make.**
