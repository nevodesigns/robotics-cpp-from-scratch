# The Loop Variable Is a Copy: Range-for, and What the Declaration Decides

> Five readings summing to 15 were doubled by a loop, and still summed to 15.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-02, 01-04, 01-11

## The Problem

Lesson 01-02 taught the loop that counts. Lesson 01-04 then wrote this, in the
code you were asked to read:

```cpp
for (const double value : samples) sum += value;
```

and nobody said what it was. It is the **range based for**, the loop that visits
each thing in turn with no index, and it appears in twenty lessons from
phase 00 onwards. Lesson 03-08 explains what a type needs so that this loop can
walk it. This is the other half, the one you meet first: the part you write.

It has one declaration in the middle of it, the loop variable, and almost every
way this loop goes wrong is in that declaration. What you write decides whether
the variable **is** the element or a copy of it, and the language will not tell
you which you meant.

## The Concept

### What the loop is

```cpp
for (declaration : container) body
```

Take each element of the container in turn, use it to initialise the variable
you declared, and run the body. The container is evaluated once. There is no
index, which is the point: no off-by-one, no unsigned counter from lesson 01-07.

Everything depends on the word *initialise*. The declaration is an ordinary
variable declaration, initialised from the element, so the rules of lesson 01-11
apply: a variable declared by value is a copy, and one declared with `&` is
another name for the thing.

### A copy, modified, thrown away

```
  five readings summing to 15, doubled two ways:
    for (double v : values) v *= 2       sum is now 15
    for (double& v : values) v *= 2      sum is now 30
```

The first loop runs. Each `v` is doubled. Each doubled `v` is destroyed at the end
of its pass, and the container is untouched.

Whether anything says so depends on the compiler and on what else the loop does.
Measured on this loop with nothing after the multiplication: gcc 11 reported
nothing, and clang 14 reported `-Wunused-but-set-variable`, because the copy was
set and never read. Add one line that reads it afterwards, `sum += v`, and both
were silent. A warning that appears only when the bug is the whole body is not
protection.

`E-CPP-0063`.

### Every element, copied

```
  1000 readings, each loop run once, copies made:
    for (Reading r : readings)            1000
    for (const Reading& r : readings)        0
    for (auto r : readings)               1000
    for (const auto& r : readings)           0
```

`Reading` counts its own copies, so this is measured without a clock. A clock
would measure the machine that day.

`auto` asks the compiler to write the type for you. The type it writes has **no
reference in it** unless you add one, so `auto r` is a copy exactly as
`Reading r` is. The `&` is the part that matters, and `auto` leaves it to you.

For a `double` the copy is free, a number is as cheap to copy as to refer to. For
a struct holding a string or a vector it is a real allocation per element, which
is lesson 01-11's cost arriving inside a loop.

### The loop variable is not the element

```
  the address of the loop variable, compared with the element's:
    for (double v : values)         different
    for (const double& v : values)  same
```

Asked from inside the loop, where the answer means something. It matters the
moment a function returns a pointer to what it found:

```cpp
for (Reading r : readings)
  if (r.value > limit) return &r;          // the address of a copy
```

That is the address of a variable that stops existing when the function returns.
The suite checks that `first_above` returns exactly the address of the right
element in the vector.

### A temporary in the range expression is gone before the body runs

```
  holders already destroyed when the loop body first ran:
    a temporary, a copy returned by value   1
    a named holder                          0
```

The thing after the colon is evaluated once. A temporary inside it that is not
the final result is destroyed before the body runs:

```cpp
for (int x : Holder().items()) { ... }     // items() returns a reference into the Holder
```

`Holder()` is a temporary and `items()` hands back a reference into it. If the
function returns a copy, **by value**, the copy is kept alive for the whole loop
and this is safe, which is what the first row measures. If it returns a reference
into the object, the loop reads something already gone.

Experiment, method stated: that version run once, outside the suite, because the
suite does not execute a read of freed memory on purpose. In a normal build it
summed to **2882482050** with no error and no crash, a number that looks like
data. Under the address sanitizer:

```
ERROR: AddressSanitizer: stack-use-after-scope on address 0x7ffd...
READ of size 8 at 0x7ffd... thread T0
```

C++23 changed the rule so that every temporary in the range expression lives for
the loop. This curriculum is C++17, where it does not, and code that is fine on a
newer compiler is a use after scope here.

`E-MEM-0019`.

### A vector of bool is not a vector of bools

```cpp
std::vector<bool> flags(3, true);
for (auto& flag : flags) count += flag;      // will not compile
```

A `std::vector<bool>` packs eight flags to a byte, so there is no `bool` to refer
to, and indexing hands back a small stand-in object that knows which bit it
means.

```
  sizeof(bool)             1
  sizeof(flags[0])         16
```

A pointer to the byte and a mask for the bit, on gcc 11. The size is the
library's choice, so the suite asserts only that it is not a bool's. A loop
variable of `auto&` wants a reference to something that exists, and the
stand-in is a temporary. Both compilers refuse it, with a message about binding
to a temporary that does not mention `vector<bool>` until you read the type in
the middle. `checks/a_reference_to_a_flag.cpp` is that refusal.

Copying the stand-in works and is cheap, `for (auto flag : flags)`. Writing to
the flags is an index loop.

`E-CPP-0064`.

### The rule

> Read with `const auto&`. Change with `auto&`. Copy only something as cheap as a
> number.

And use an index when you need the position, or need to change the container's
size while you walk it. Lesson 02-07 is what happens to a loop that does.

## Build It

Six functions, each short.

`scale_all` changes every reading in place, so its loop variable is a reference.
`total` and `count_above` only read, so theirs is a `const` reference, and the
suite counts the copies and requires none. `first_above` returns a pointer to the
element, so the reference is not optional.

`count_true` copies the loop variable, which is right for the reason in the last
section. `mark_all` cannot use a reference at all and is an index loop with an
unsigned counter, with everything lesson 01-07 said about that.

The stubs do nothing and return zero. The suite asks about one reading as well
as none, because a stub that returns zero would otherwise give the right answer
for an empty container.

## Use It

```cpp
for (const auto& reading : readings) total += reading.value;     // read
for (auto& reading : readings) reading.value *= factor;          // change
for (double sample : samples) sum += sample;                     // cheap to copy
```

Two limits.

**This does not make a loop safe against changing the container while walking
it.** Adding or removing elements inside the body can invalidate what the loop is
holding, which is lesson 02-07.

**A signal that a loop is slow is not a signal that it copies.** The count shows
that it does. Whether the copy matters is a question for lesson 04-05, which
measures where time goes.

## What Breaks First

`E-CPP-0063` is the loop that changed nothing, the pointer to a copy, and the
loop that was slow because it copied a thousand structs.

`E-CPP-0064` is the `auto&` that will not compile on a `vector<bool>`, with an
error that does not say why.

`E-MEM-0019` is the loop over something that was already gone, which gave a
believable number in a normal build and was found only by the address sanitizer.

## Ship It

No header graduates from this lesson. Its output is a habit, one declaration
chosen on purpose, and the three spellings in the last section.

From here on, a range based for in this curriculum reads with `const auto&`,
changes with `auto&`, and copies only what is as cheap as a number.
