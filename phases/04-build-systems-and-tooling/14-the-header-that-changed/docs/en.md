# The Header That Changed and Took Everything With It: Hiding a Class's Insides So Its Size Does Not Change

> A field is added to one class. Every file that includes it builds again, and a library built before the change
> can corrupt memory when linked with code built after. The cure is to keep the insides out of the header.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-04, 02-05, 04-01

## The Problem

Lesson 04-01 showed why a build rebuilds the files that depend on a changed header. A class in a widely used header is the worst case: adding a private field changes the header, and every file that includes it is built again.

Rebuilding is only the visible cost. A library compiled against the old header has the old class's size in it. Link it with code compiled against the new header and the two disagree about how big an object is, and the program damages memory somewhere nowhere near the change.

## The Concept

### The size of a class is in every caller

A function that holds an object of a class reserves `sizeof` bytes for it, worked out when that function was compiled. The same data written two ways, in two versions of the header:

| data | version 1 | version 2 (adds a history of ten readings) |
|---|---|---|
| in the class | 24 bytes | 104 bytes |
| behind a pointer | 8 bytes | 8 bytes |

Code built against the version 1 header reserves 24 bytes for something that now needs 104. See `E-BUILD-0020`.

### Hide the inside

```cpp
class Smoother {
  ...
 private:
  struct Impl;                    // named, not defined
  std::unique_ptr<Impl> impl_;
};
```

A `Smoother` is **8** bytes, the size of one pointer, whatever is in `Impl`. The fields are defined where only the implementation sees them. Add one and nothing a caller was built against changes. This is called the **pimpl** idiom, for "pointer to implementation".

### The destructor goes where the type is complete

A `unique_ptr` must know the size of what it deletes. If the class does not declare a destructor, the compiler writes one at the end of the class, where `Impl` is still only a name. The build fails with `invalid application of 'sizeof' to incomplete type 'Hidden::Impl'`. The lesson ships a snippet that does it, and the build tries it and expects to fail, so the guarantee is checked and not remembered.

The fix is to declare the destructor in the class and define it, even as `= default`, after `Impl` is complete. The move operations need the same. See `E-BUILD-0021`.

### A copy has to be made

A `unique_ptr` has no copy, so a class holding one has none either. The copy constructor makes a new `Impl` from the old:

```cpp
Smoother::Smoother(const Smoother& other) : impl_(new Impl(*other.impl_)) {}
```

A filter at 15 after two readings was copied, and a reading of 20 gave the copy 17.5. The original, given 0, then gave **7.5**: it had not moved, since the two do not share. Assigning one to another, assigning to itself and moving all behaved. See `E-BUILD-0022`.

## Build It

The constructor, copy constructor, copy assignment, `update` and `count` in `exercise/solution.hpp`. The class declaration, `Impl` and the defaulted members are given.

```
rcpp verify 04-14
```

## Use It

`rc/core/smoother.hpp` joins `rc::core`. In a real library the first part of the file is the header, and `Impl` and the definitions are in a `.cpp`.

```cpp
Smoother left(0.2), right(0.2);
double a = left.update(reading_from_left);
double b = right.update(reading_from_right);
```

**What this does not do.** It costs a heap allocation for each object and an extra step to reach every field, which a control loop at a kilohertz will notice if it makes objects in the loop (lesson 02-14 has the pool for that). It hides the layout from the compiler too, so a call cannot be inlined across the boundary unless
the whole program is optimised together. It only helps if the header really is where the change was: a change to a public function still rebuilds every caller. And it does not make a binary change safe for other reasons, such as a virtual function added in the middle.

## What Breaks First

- **A field added to an open class.** Every caller reserves the wrong size. See `E-BUILD-0020`.
- **A compiler written destructor.** The build fails inside the standard library. See `E-BUILD-0021`.
- **A shared or missing copy.** Two filters on one state, or none. See `E-BUILD-0022`.

## Ship It

`rc/core/smoother.hpp` holds `Smoother`.

Phase 04's end is a library installed and consumed by a project that has never seen its source. This is how that library can change its inside without the project noticing.
