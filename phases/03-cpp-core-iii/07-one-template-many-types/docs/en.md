# One Template, Many Types: A Ring Buffer That Works for Anything

> The same mistake, two compilers. gcc went from 113 lines of error to 5 when
> the template was constrained. clang went from 224 to 254.

**Type:** Build
**Time:** about 135 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-02, 03-01

## The Problem

This phase is called classes and templates, and so far it has been classes.

A robot keeps the last N of things. The last twenty encoder readings to smooth.
The last hundred poses to draw a trail. The last fifty loop times to find the
slow one. Always the last N, never all of them, because a machine that runs for
a week cannot keep a week of samples.

You could write that once for `double`. Then again for `int`. Then again for a
pose. Or you could write it once.

## The Concept

### Two kinds of template parameter

```cpp
template <class T, std::size_t Capacity>
class Ring { ... };
```

`T` is a **type parameter**, which is what everybody means by "template".

`Capacity` is a **non type parameter**: a number known while the code is being
compiled. That distinction is not decoration. Because the capacity is a
compile time constant, the storage can be a plain array inside the object:

```cpp
T storage_[Capacity] = {};
```

Measured, 1000 pushes into a `Ring<double, 64>`:

| | |
|---|---|
| allocations | **0 blocks, 0 arrays** |
| `sizeof(Ring<double, 64>)` | 528 bytes, of which 512 are the doubles |

The other 16 bytes are two indices. Nothing on the heap, nothing copied on a
push, nothing shifted. That is the reason to write this rather than reach for a
`std::vector`, and it is why a control loop can hold one.

### Each instantiation is a separate class

This is the thing most worth understanding about templates, and it is easy to
measure.

`Ring<double, 4>` and `Ring<int, 4>` share a definition and nothing else.
Pushing into one leaves the other at size zero. And `sizeof(Ring<double, 8>)`
is 80 against 48 for `Ring<double, 4>`, because they are different classes with
different arrays in them.

The suite makes it visible with a counter per type:

```
Instantiated<double>::uses 5
Instantiated<int>::uses    2
```

Two classes, not one class used twice. Which is also the honest cost of
templates: a template in a header is compiled again for every type and every
translation unit that uses it.

### The one line that is the whole idea

```cpp
std::size_t slot(std::size_t index) const { return (first_ + index) % Capacity; }
```

Positions count from the oldest element, and **the oldest moves**. So a
position and an array slot are not the same number, and the remainder brings it
back round.

Everything else in the class follows from that. `push` has two cases: there is
room, or there is not, and when there is not, the slot holding the oldest
element is the one to reuse because it is the one you are willing to lose.

Pushing 1 to 7 into a ring of 4:

| pushed | size | holds | oldest | newest |
|---|---|---|---|---|
| 4 | 4 | 1 2 3 4 | 1 | 4 |
| 5 | 4 | 2 3 4 5 | 2 | 5 |
| 7 | 4 | 4 5 6 7 | 4 | 7 |

`oldest()` is 4, not 1. It is the oldest reading **still held**.

Note what the suite tests: a hundred pushes into a ring of three, and a ring of
one. Those are where the off by one errors live, and a test with fewer pushes
than the capacity cannot see any of them.

### A template has no declared interface

Here is the part that makes templates unpleasant to learn, and the reason it is
worth measuring rather than describing.

A class or a function template is checked against **whatever its body happens
to use**. There is no list of requirements anywhere. So an unsuitable type does
not fail at the call; it fails wherever the body first touches something that
type lacks, which may be several layers down inside somebody else's header.

Measured, calling `std::sort` on a vector of a type with no `operator<`:

| | gcc | clang |
|---|---|---|
| no constraint | 113 lines, 12884 chars | 224 lines, 24069 chars |
| constrained with a trait | **5 lines**, 509 chars | **254 lines**, 25648 chars |

The unconstrained error's first line names
`/usr/include/c++/11/bits/predefined_ops.h`, **21 instantiation frames** deep
in a file you have never opened.

Now read the table again, because the obvious conclusion is half wrong. gcc
stops at the failed assertion and the diagnostic collapses. **clang reports the
assertion and then instantiates the rest anyway, so the total goes up.**

So "static_assert makes template errors short" is not true. What it reliably
does on both compilers is fix the **first error line**, which becomes a
sentence you wrote, naming your own file:

```
trait_guarded.cpp:8:40: error: static assertion failed: sort_numbers() needs
numbers. Pass int, double or similar, or call std::sort with your own comparator.
```

against

```
/usr/include/c++/11/bits/predefined_ops.h:45:23: error: no match for 'operator<'
```

The first line is the one that gets read. Spend the sentence on what to pass
instead, not on what the compiler could not do.

### Where to put the constraint

At **your own boundary**, in your own template:

```cpp
template <class T, std::size_t Capacity>
double average(const Ring<T, Capacity>& ring) {
  static_assert(std::is_arithmetic<T>::value,
                "average() needs numbers. Ring<double, N> or Ring<int, N> work. "
                "For your own type, average the member you mean.");
```

Putting it deeper does not help, because deeper is where the problem already
was. And prefer a trait from `<type_traits>` that already exists:
`std::is_arithmetic`, `std::is_integral`, `std::is_same`. Writing a detection
idiom by hand in C++17 costs twenty lines to say what one of those says.

The same technique refuses a ring with no room:

```cpp
static_assert(Capacity > 0, "a Ring needs room for at least one element. ...");
```

`Ring<double, 0>` is not a smaller ring. Every operation would be an error and
the modular arithmetic would divide by zero. Both of these are in `checks/` as
code that must not compile, so you meet the messages deliberately.

## Build It

Implement `slot`, `push`, `operator[]` and `average` in
`exercise/solution.hpp`.

```
rcpp verify 03-07
```

The suite pushes past the capacity and prints what the ring holds at each step,
runs a hundred pushes through a ring of three, exercises a ring of one, serves
three different element types including a struct of its own, counts
instantiations, and proves the whole thing allocates nothing. Two snippets in
`checks/` are compiled and expected to fail.

## Use It

**Reach for `rc::core::Ring` for any fixed size window.** The size is in the
type, it allocates nothing, and it overwrites the oldest element in constant
time.

**Never build a window out of `push_back` and `erase(begin())`.** That is an
allocation and a full shift per cycle, and it invalidates every pointer into the
buffer, which lesson 02-07 covered.

**Test past the capacity, and test a capacity of one.** Those two tests catch
every wrap bug there is.

**Constrain your templates at your own edge**, with a trait that already
exists, and write the message as an instruction.

**Read the first error line.** Then find the shallowest frame in your own code
in the "required from" chain. Everything else is the compiler explaining how it
got there.

## What Breaks First

- **An error reported from inside a library header.** See `E-CPP-0040`.
- **A ring that loses track of the oldest element.** See `E-CPP-0041`.
- **A history buffer that allocates on every reading.** See `E-CPP-0042`.

## Ship It

`Ring` and `average` join `rc::core`. This is the generic ring buffer phase 03
was planned around, and the rest of the curriculum keeps its last N readings in
it: the moving windows in phase 06, the latency histories in phase 07, and the
pose trails in phase 16.
