# How Many Copies: Counting What a Vector Does, and the One Word That Keeps It From Copying

> Copies do not show in the source. A type that counts its own copies and moves shows
> which lines make them, and one word decides whether a vector copies every element every time it grows.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-04, 02-05

## The Problem

A program that stores a few thousand objects runs slower than the arithmetic says it should. The output is right and nothing leaks. Each object is cheap to make, and the program is making far more of them than the
code seems to ask for.

Lesson 02-05 explained copying and moving. What it could not do is show how often they happen, because a copy is not on any line of the source. It is in the passing of a parameter, the growing of a vector, the return of a value.

## The Concept

### A type that counts itself

`Packet` has one number in it and five special members: a constructor, a copy constructor, a move constructor, two assignments and a destructor. Each adds one to a counter. After that, a copy is not invisible.

```cpp
struct Tally { int made; int copies; int moves; int destroyed; };
```

### The word on the move constructor

A vector that is full allocates a bigger block and carries its elements across. It moves them only if the move constructor is declared `noexcept`. A move that might throw half way would leave some elements in the old block and some in the new, and
the vector cannot undo that, so if it cannot be sure it copies.

A hundred push_backs with no reserve, on libstdc++:

| move constructor | copies | moves |
|---|---|---|
| marked `noexcept` | 0 | 227 |
| not marked | 127 | 100 |

The class is identical except for one word. For an object that owns a buffer, 127 copies is 127 allocations. See `E-MEM-0026`.

### Say how many there will be

`reserve(n)` allocates once. The moves of growth go, leaving one per `push_back` of a temporary:

| way of filling | copies | moves |
|---|---|---|
| no reserve | 0 | 227 |
| reserve, `push_back(Packet(i))` | 0 | 100 |
| reserve, `emplace_back(i)` | 0 | 0 |

`emplace_back` passes the arguments to the constructor and builds the object where it will live, so there is nothing to move. The growth count depends on how much the library grows by each time, so a test should check that reserving removes it and not how large it was. See `E-MEM-0028`.

### Passing a vector

A vector of 50 packets, passed to a function that adds up their ids:

| how | copies |
|---|---|
| by value | 50 |
| by const reference | 0 |
| by value, from `std::move` | 0, and 0 moves of elements |

A named vector passed by value is copied, every element. A vector given away with `std::move` hands over its buffer in one step. Returning a vector from a function made 0 copies and 0 moves. See `E-MEM-0027`.

## Build It

The five special members of `Packet` in `exercise/solution.hpp`, with the counts and the word `noexcept`. The three builders and the two totals are given.

```
rcpp verify 02-15
```

The test compares your `Packet` with a `Careless` that is the same class without the promise, and checks `std::is_nothrow_move_constructible`.

## Use It

`rc/core/tally.hpp` joins `rc::core`. Put the counting in a type of your own and run the code you suspect:

```cpp
reset_tally();
std::vector<Packet> v = build_emplaced(100);
// tally().copies and tally().moves say what happened
```

**What this does not do.** It counts the constructions and not their cost: a copy of a number is nothing and a copy of a buffer is an allocation. The numbers for growth are those of libstdc++, and other libraries grow by other factors, so look at the
comparison and not at 227. The compiler can remove a copy or a move altogether (copy elision), so a count of zero may be the compiler's work and not the code's. And it says nothing about memory use, only about how often objects are made.

## What Breaks First

- **A move constructor without `noexcept`.** The vector copies on growth. See `E-MEM-0026`.
- **A vector taken by value.** Every element copied. See `E-MEM-0027`.
- **`push_back` of a temporary with no reserve.** Moves for nothing. See `E-MEM-0028`.

## Ship It

`rc/core/tally.hpp` holds `Tally`, `tally`, `reset_tally`, `Packet` and the three builders.

Phase 07 asked about allocation in a loop, and lesson 02-14 made it zero. This is how to find out where it was coming from.
