# Declare Nothing, or Declare Everything: What One Destructor Costs

> 198 allocations where there had been 71. The commit that did it added
> `~X() = default;` to a struct, a line that does nothing at all.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-11, 02-05, 03-07

## The Problem

Lesson 02-05 had you write the five special member functions by hand, once, so
that what a resource owning type has to get right is something you have done
rather than something you have read about.

This lesson is the other half of that, and the conclusion of 02-05 is not what
most people take from it.

> Almost never do that again.

The compiler writes all five for you, correctly, as long as every member knows
how to look after itself. Declaring even one of them by hand changes what it
writes for the others, and the change is silent.

## The Concept

### Four classes holding the same thing

```cpp
struct Quiet          { std::vector<double> samples; };
struct WithDestructor { std::vector<double> samples; ~WithDestructor() = default; };
struct WithCopy       { std::vector<double> samples; WithCopy(const WithCopy&); };
struct AllFive        { std::vector<double> samples; /* all five, = default */ };
```

Measured, by filling one with a buffer, moving it, and comparing where the
buffer lives afterwards:

| class | moves cheaply | buffer on move |
|---|---|---|
| nothing declared | yes | travelled |
| `~X() = default;` declared | **no** | **copied** |
| copy constructor declared | **no** | **copied** |
| all five declared | yes | travelled |

Read the second row again. The destructor is written `= default`. It does
nothing whatever. It is as close to not being there as the language allows.
And it suppresses the implicit **move** constructor, so every move of the class
is quietly a copy.

The copy operations are still generated, so nothing fails. The class still
works. It is just slower.

### What that costs in a program

A trait is not a number. Pushing 64 rows, each holding 64 doubles, into a
`std::vector`:

| element type | allocations |
|---|---|
| nothing declared | 71 |
| `~X() = default;` declared | **198** |
| all five declared | 71 |

**127 extra allocations from a line that does nothing.**

Every time the outer vector runs out of room it has to relocate its rows.
Moving one steals its buffer and allocates nothing. Copying one allocates a new
buffer for every row it relocates, every time it grows.

### The trait everybody checks is useless here

The obvious thing to assert is `std::is_move_constructible`. Measured:

| class | `is_move_constructible` | `is_nothrow_move_constructible` |
|---|---|---|
| nothing declared | yes | yes |
| `~X() = default;` declared | **yes** | **no** |
| copy constructor declared | **yes** | **no** |

It says **yes to all three**, including the two that have no move constructor
at all. Because it asks whether a `T` can be constructed from an rvalue `T`,
and a copy constructor can do that: binding a `const T&` to an rvalue is
perfectly legal.

So the trait is satisfied by copying and cannot tell you this.

The nothrow variant matched the observed behaviour on every row, and it is the
right question for a reason rather than by coincidence: **it is the condition
the standard library itself branches on.** A `std::vector` reallocating has
already allocated the new buffer and is part way through relocating its
elements. If a move could throw, it could not put them back, so it copies
instead and keeps the strong exception guarantee.

Which means a class that is move constructible and not *nothrow* move
constructible is copied by every container it is put into, while the trait most
people check reports no problem.

### The asymmetry that makes this dangerous

Declaring a move constructor suppresses the implicit **copy** constructor. That
is also surprising, and it is **refused at the call site**, because the caller
tries to use something that was taken away. You find out immediately.

Declaring a destructor suppresses the implicit **move** constructor. Nothing
complains, because it took away something the compiler was going to do on your
behalf and nobody asked for it by name.

Same rule, and only one of the two tells you. This lesson keeps the loud one in
`checks/` as code that must not compile, and the quiet one as the measurement
above.

### So assert it

```cpp
static_assert(std::is_nothrow_move_constructible<Readings>::value,
              "Readings must be cheap to move. Declaring a destructor "
              "suppressed its move constructor, so every container of these "
              "copies on every growth. Remove the destructor, or declare all "
              "five.");
```

One line at the top of a header, and the next person to add a destructor finds
out immediately rather than from a profile six months later. The message is an
instruction rather than a complaint, because the first line of a failed
`static_assert` is the one that gets read, which lesson 03-07 measured.

### Why not just declare all five

Because it works and it is not the advice.

Measured, `AllFive` is identical to `Quiet` on every count: the same trait, the
same buffer travelling on a move, the same 71 allocations. It cost six lines.

Those six lines now have to stay correct as the class changes. Add a member and
five of them are places that can drift from the member list, which is every bug
lesson 02-05 wrote the operations by hand to meet: a member copied by the copy
constructor and not by the copy assignment, a self assignment that empties the
object, a moved from object in a state the destructor does not expect.

`Quiet` has nothing to keep in step with anything. Add a member and all five
adjust.

**Write them out only when the class owns a raw resource**: a file descriptor,
a device handle, a C API object, memory from `malloc`. There the rule of five
applies and `rc/core/buffer.hpp` from 02-05 is what it looks like.

**Better still, wrap the raw thing once and go back to zero.** A
`std::unique_ptr` with a custom deleter, or a small handle like
`rc/io/handle.hpp` from 02-03, owns the awkward part, and everything built on
top of it declares nothing again. One type that knows about the resource, and
all the rest written in the easy style.

## Build It

Implement `moves_cheaply`, `buffer_travels_on_move` and `allocations_to_grow`
in `exercise/solution.hpp`.

```
rcpp verify 03-09
```

The suite asks both traits about all four classes, moves one of each and
watches where the buffer goes, counts the allocations growing a container of
each, and shows that declaring all five puts everything back. Two snippets in
`checks/` are compiled and expected to fail.

## Use It

**Declare nothing.** If the members look after themselves, and
`std::vector`, `std::string` and `std::unique_ptr` all do, the compiler is
right and stays right.

**Never add a destructor for logging.** That is the commonest way this happens.

**If you declare one, declare all five, with `= default`.**

**Assert `is_nothrow_move_constructible` on any type that goes into a
container**, and never assert the plain `is_move_constructible`, which cannot
fail.

**Do not add `noexcept` to make the trait pass.** Marking a move that can throw
as `noexcept` does not make it safe, it makes a throw terminate the program.

**Count the allocations when you want to be sure.** Four lines with
`rc::test::AllocationCount`, and it turns a trait into 71 against 198.

## What Breaks First

- **A destructor that silently stopped the class moving.** See `E-CPP-0049`.
- **`is_move_constructible` trusted to mean something.** See `E-CPP-0050`.
- **Five hand written members that drift apart.** See `E-CPP-0051`.

## Ship It

This lesson ships no header. What it ships is a rule and a one line assertion,
and the reason `rc::core::Ring`, `rc::core::SectionTimer` and most of the rest
of librc declare none of the five.
