# Behaviour as a Value: What a Lambda Captures, and For How Long

> Returning a reference to a local is refused by both compilers. Hide the same
> reference inside a lambda's capture list and both accept it without a word.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-11, 02-02, 02-07

## The Problem

You have already been handed these. Lesson 03-04 passes four of them to
standard algorithms:

```cpp
std::count_if(readings.begin(), readings.end(),
              [](const Reading& r) { return r.valid; });
```

Nothing has explained what the square brackets at the front mean. They are the
capture list, and they are the whole subject of this lesson.

They are also why this lesson is in the phase about ownership rather than the
one about values. Capturing by value takes a copy and owns it. Capturing by
reference borrows, and lesson 02-07 was about what borrowing costs when the
thing you borrowed from goes away.

## The Concept

### A lambda is a class the compiler writes

That is the whole of it. `[limit](double v) { return v > limit; }` is a class
with one member, `limit`, and a call operator. The capture list is its member
list and its constructor.

Everything else follows from that sentence, including the sizes:

| what it captures | sizeof |
|---|---|
| nothing | 1 |
| one double | 8 |
| one reference | 8 |
| a whole vector | 24 |

`sizeof(std::vector<double>)` is 24. The lambda that captured one is exactly
that big, because it has one as a member.

The empty one is 1 byte rather than 0 because an object has to have a distinct
address, and a class with no members still has to obey that.

### Two captures, two different questions

Both of these were created while the limit was 1.0. Then the limit became
100.0. Asked whether 50 exceeds it:

| capture | answer |
|---|---|
| `[limit]`, by value | **true**, still answering against 1.0 |
| `[&limit]`, by reference | false, reading 100.0 at the call |

Neither is wrong. They are different things.

> A by value capture is a decision taken once.
> A by reference capture is a question asked every call.

A limit somebody can turn wants the second. A limit frozen at the moment a
callback was registered wants the first. One character separates them and it is
easy to read past, so prefer naming each capture over `[=]` or `[&]`: it is one
word longer and says exactly what the lambda depends on.

### The capture list is a list of initialisations

Which means everything lesson 01-11 measured applies unchanged. Capturing a
container by value copies it:

| capture | what the callable reads |
|---|---|
| `[samples]` | a copy of the buffer |
| `[&samples]` | the same buffer |

A captured `std::vector` of 1024 doubles gave the callable its own buffer at a
different address. Nothing in the square brackets says "copy a kilobyte", and
that is exactly what it did.

### The mistake with no diagnostic

```cpp
std::function<double()> latest_reading() {
  double reading = read_sensor();
  return [&reading]() { return reading; };    // reading dies here
}
```

The reference points into a stack frame that is about to be reused. This is
lesson 02-07 with a different shape: the borrowed thing went away and the
borrower did not find out.

Measured with `-Wall -Wextra -Werror`: **gcc accepted it and clang accepted
it.**

Compare that with returning a reference to a local directly, which both
compilers refuse and which lesson 01-11 keeps in its `checks/` directory. The
same reference, hidden inside a capture list, produces nothing at all.

This is a pattern worth naming, because this curriculum keeps running into it:
**the compiler catches the obvious form and says nothing about the form people
actually write.** The address sanitiser does find this one at runtime, which is
why the lane exists.

When a reference capture is genuinely what you want, write the precondition
above the signature. It is the only check there is going to be.

### Every lambda has its own type

Two character for character identical lambda expressions are two different
types. The compiler writes a new class for each one.

That is why you store one with `auto`: the type has no name you could write
down. And it is why putting one into a `std::function` is a conversion rather
than a copy, which brings us to the last measurement.

### What std::function costs

`std::function` erases the type of what it holds, and it has to put that
somewhere. It keeps a small buffer inside itself and anything that does not fit
goes on the heap. With `sizeof(std::function<std::size_t()>)` of 32:

| bytes captured | allocations to store it |
|---|---|
| 8 | 0 |
| 16 | 0 |
| **17** | **1** |
| 64 | 1 |
| 64, called directly with no `std::function` | **0** |

**The lambda never allocates.** 64 bytes of captures is a 64 byte object on the
stack. The allocation comes from the type erasure, and it happens when the
`std::function` is assigned, not when it is called.

The size it holds for free is a standard library choice, not a rule, so the
suite prints the sweep rather than asserting the number.

This lesson's own two callables land on opposite sides of that line:

| | allocations to store |
|---|---|
| captures one double | **0** |
| captures a whole vector | **3** |

Three, because the vector's buffer was copied into the lambda and then the
lambda was too big to live inside the `std::function`. Two costs for one
capture list.

### And the one a C API will take

A lambda that captures nothing converts to a plain function pointer, because
there is no state to carry. One that captures anything does not, because the
pointer has nowhere to put the captured values.

That is the error a C callback registration produces, and the fix is a `void*`
user pointer or a `std::function`, not a cast. It is in `checks/`, along with
modifying a by value capture without `mutable`.

## Build It

Implement `watching_a_copy`, `watching_the_original`,
`address_of_a_copied_buffer` and `address_of_a_borrowed_buffer` in
`exercise/solution.hpp`.

```
rcpp verify 02-08
```

The suite changes a limit after the callables were made and asks both what they
think, compares buffer addresses across both capture forms, measures the size
of four lambdas, proves two identical ones are different types, and sweeps the
`std::function` allocation threshold. Two snippets in `checks/` are compiled
and expected to fail.

## Use It

**Name each capture.** `[limit]` or `[&limit]`, not `[=]` or `[&]`. A catch all
list makes a reader scan the body, and it silently picks up whatever a later
edit happens to reference.

**Capture by value anything that outlives the scope**, and write the
precondition down when you deliberately do not.

**Remember a container capture copies the container.** Capture a pointer to the
data, after answering the lifetime question that raises.

**Build a `std::function` outside the loop**, because the allocation is at
assignment.

**Prefer calling a lambda directly, or passing it as a template parameter.**
No type erasure, no allocation. That is how the standard algorithms take
theirs.

**Count the allocations.** `rc::test::AllocationCount` turns "this callback
should not allocate" into an assertion.

## What Breaks First

- **A copy captured where the original was needed.** See `E-CPP-0043`.
- **A lambda that outlived what it captured by reference.** See `E-CPP-0044`.
- **A std::function in a control loop that allocates.** See `E-CPP-0045`.

## Ship It

This lesson ships no header. What it ships is the vocabulary: from here on the
curriculum may use lambdas in the code you read, and the auditor enforces that,
because rule L021 now tracks them and will refuse any lesson that uses one
without this in its prerequisite chain. Lessons 03-04 and 11-01 were already
using them, and now say so.
