# Pointers: Talking About Where Something Is

> A pointer is not a mysterious kind of number. It is the answer to the question "where is it?", and the whole difficulty is that the answer can go out of date.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-04

## The Problem

You have a hundred sensor readings and you want the first one above a limit.

Returning the value is not enough, because the caller usually wants to know
*which* reading it was, so it can look at its timestamp or mark it. Returning the
index means inventing a value that means "nothing found", and every codebase that
does this picks a different one: minus one, or the count, or zero, and somebody
eventually forgets to check.

There is a third answer. Hand back the location of the reading, or hand back
nothing at all. That is what a pointer is for, and `nullptr` is the "nothing at
all" that the language itself understands.

## The Concept

Memory is a very long row of numbered boxes. Every object your program creates
lives in some of those boxes, and the number of the first one is its **address**.

A **pointer** is a variable that holds an address:

```cpp
double reading = 3.5;
double* where = &reading;   // & means "the address of"
double copy = *where;       // * means "the value living at that address"
```

Two symbols, both overloaded, which is a genuine wart in the language:

- In a **type**, `double*` means "a pointer to a double".
- In an **expression**, `&x` means "the address of x" and `*p` means "the value
  at p".

### Why pointers exist at all

Three reasons, and every use you meet is one of them:

1. **To say nothing.** A pointer can be `nullptr`, which is a real answer meaning
   there is no such thing. A reference cannot.
2. **To talk to hardware.** A memory mapped register lives at a fixed address.
   You will do exactly this in phase 08.
3. **To refer without copying.** Though for this a reference is usually clearer,
   which is the rule from lesson 01-03.

### nullptr, not NULL and not 0

Older code writes `NULL` or plain `0`. Both are the number zero wearing a
disguise, and both cause real ambiguity when a function is overloaded on `int`
and on a pointer. `nullptr` has its own type and cannot be mistaken for a number.

This curriculum never uses `NULL`, and rule L018 in the lesson contract rejects
it, because a learner who meets it in a lesson will assume it is current practice.

### The rule that makes pointers safe

**A pointer is only as valid as the thing it points at.**

That sentence is the entire subject of the next lesson and most of this phase.
An address does not keep anything alive. When the object at that address is
destroyed, the pointer still holds the same number, and that number now refers
to memory that belongs to something else. Reading it is undefined behaviour, and
the worst part is that it usually appears to work.

So whenever you hold a pointer, you must be able to answer: what keeps this
alive, and for how long?

### Checking before dereferencing

A pointer that can be null must be checked before it is used:

```cpp
const Reading* found = find_first_above(readings, count, 80.0);
if (found != nullptr) {
  use(found->celsius);
}
```

Skipping the check is not a style mistake. Dereferencing a null pointer is a
crash on every platform this curriculum supports, and it is the second most
common cause of a robot process dying in the field.

## Build It

`exercise/solution.hpp` gives you a `Reading` type. Implement:

- `find_first_above(const Reading* readings, int count, double limit)` returns
  the address of the first reading above the limit, or `nullptr` when there is
  none. It must also return `nullptr` when handed a null array or a count of
  zero, rather than reading from nowhere.
- `swap_readings(Reading* a, Reading* b)` exchanges two readings through
  pointers, and does nothing at all if either pointer is null.
- `highest(const Reading* readings, int count)` returns the address of the
  largest reading, or `nullptr` when there is none.

```
rcpp verify 02-01
```

## Use It

The standard library does this with iterators rather than raw pointers.
`std::find_if` returns an iterator, and the "nothing found" answer is `end()`
rather than `nullptr`. It is the same idea with a different spelling, and for a
plain array a pointer genuinely is an iterator.

C++17 also offers `std::optional`, which expresses "a value or nothing" without
any address at all, and it is the better answer whenever you want to hand back a
*value* rather than a location. You will use it from phase 03 onward.

Raw pointers keep their place in exactly two situations: talking to hardware and
to C libraries, and referring to something whose lifetime is obviously longer
than the reference. Everything else in this curriculum uses a reference, an
optional, or one of the owning types from lesson 02-04.

## What Breaks First

- **Your program crashes the moment nothing is found.** You dereferenced the
  result without checking it against `nullptr`. See `E-MEM-0003`.
- **A function that takes a pointer changed nothing.** You reassigned the pointer
  itself rather than the value it points at. Assigning to `p` moves the pointer;
  assigning to `*p` changes the thing. See `E-CPP-0006`.
- **A returned pointer holds nonsense.** It pointed at a local variable that was
  destroyed when the function returned. See `E-MEM-0001`.

## Ship It

These three helpers join `rc::core`. More importantly, the habit joins you:
every pointer that can be null gets checked, and every pointer you hold has an
answer to the question of what keeps it alive.
