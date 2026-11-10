id: E-BUILD-0021
title: A hidden type whose destructor the compiler wrote too early
match: a Smoother is as big as one pointer, however much is inside it
match: rejects.a_destructor_the_compiler_wrote_too_early
platforms: linux, windows
teaches: 04-14-the-header-that-changed-and-took-everything-with-it
---

## Symptom

A class with a `std::unique_ptr` to a hidden type fails to compile, in a header, with an error about `sizeof` and an incomplete type, pointing into the standard library. The class looks right.

## Cause

A `unique_ptr` has to delete what it holds, and to delete something it must know its size. If the class does not declare a destructor, the compiler writes one at the end of the class, where the hidden type is still only a name.

The lesson ships a snippet that does exactly this, and the build is expected to fail, which it does: `invalid application of 'sizeof' to incomplete type 'Hidden::Impl'`.

## Fix

Declare the destructor in the class, and define it, even as `= default`, after the type is complete:

```cpp
~Smoother();                              // in the class
inline Smoother::~Smoother() = default;   // after Impl is defined
```

The same goes for the move operations, which are also written by the compiler.
