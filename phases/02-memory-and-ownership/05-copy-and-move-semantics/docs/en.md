# Copy and Move: Writing the Five Operations Once, So You Never Have To Again

> The point of this lesson is not that you will write these five functions often. It is that you will read them, understand what the compiler generates for you, and then almost never write them again.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-04

## The Problem

A class holds a buffer of sensor samples on the heap. Somebody writes:

```cpp
SampleBuffer a(1024);
SampleBuffer b = a;
```

The compiler generates a copy constructor for you, and what it generates copies
each member. The member is a pointer. So now two objects hold the same address,
both destructors free it, and the program corrupts its own heap. It usually
survives long enough to crash somewhere unrelated.

This is the single most expensive default in the language, and the fix is to
understand exactly which functions the compiler writes on your behalf, and when.

## The Concept

### The five operations

For any class, the compiler may generate five special member functions:

| Operation | Signature | Job |
|---|---|---|
| Destructor | `~T()` | release what is held |
| Copy constructor | `T(const T&)` | build a new one from an existing one |
| Copy assignment | `T& operator=(const T&)` | replace this one's contents with another's |
| Move constructor | `T(T&&)` | build a new one by taking from a temporary |
| Move assignment | `T& operator=(T&&)` | replace contents by taking from a temporary |

The generated versions act member by member. For an `int` that is right. For a
`std::vector` that is right, because vector defines its own. For a raw owning
pointer it is catastrophically wrong, because copying the pointer copies the
address rather than the thing it points at.

### The rule of zero

The best number of these functions to write is none.

If every member manages itself, a `std::vector` here, a `std::string` there, a
`std::unique_ptr` for a single owned object, then the compiler generated versions
are all correct, and correct for free. That is called the **rule of zero**, and
it is the shape almost all of your classes should have.

### The rule of five

If you do manage a resource directly, then writing one of the five means you must
consider all five. They are a set: the destructor releases, the copy operations
must decide what a second owner means, and the move operations must leave the
source safe to destroy.

Declaring a destructor also **suppresses** the generated move operations, so a
class that writes a destructor and stops silently loses moves and falls back to
copying. That is a performance cliff with no diagnostic at all, and it is the
usual reason a class that looks fine is slow.

### Deep copy, and the two things people get wrong

A copy must duplicate what is owned:

```cpp
SampleBuffer(const SampleBuffer& other)
    : size_(other.size_), data_(new double[other.size_]) {
  std::copy(other.data_, other.data_ + size_, data_);
}
```

Copy **assignment** is harder than copy construction, because the target already
owns something, and because the source might be the target:

```cpp
buffer = buffer;      // must not free its own data and then copy from it
```

Self assignment looks absurd until it arrives as `a[i] = a[j]` with equal
indices, or through two references to the same object. The reliable way to handle
both problems at once is **copy and swap**: build a copy, then swap with it, and
let the destructor of the temporary clean up the old contents. It is correct
under self assignment automatically, and it is exception safe, because the risky
allocation happens before anything is modified.

### Move must leave the source destructible

```cpp
SampleBuffer(SampleBuffer&& other) noexcept
    : size_(other.size_), data_(other.data_) {
  other.data_ = nullptr;
  other.size_ = 0;
}
```

Taking the pointer is not enough. Clearing the source is the important half,
because its destructor will still run, and it must not free what you just took.

Mark move operations `noexcept`. `std::vector` will only move its elements while
growing if their move constructor promises not to throw, and otherwise copies
them instead. A missing `noexcept` is a silent, measurable slowdown.

## Build It

Implement `SampleBuffer` in `exercise/solution.hpp`, all five operations plus the
constructor:

- The constructor allocates `size` doubles, zeroed.
- The destructor releases them.
- The copy operations produce an independent buffer with the same contents.
- Copy assignment survives self assignment.
- The move operations transfer the buffer and leave the source empty, and are
  marked `noexcept`.
- `size()`, `at(i)` and `set(i, value)` are provided.

```
rcpp verify 02-05
```

The tests check independence, self assignment, the moved from state, and that
allocations balance in every case.

## Use It

Now delete it and use `std::vector<double>`.

That is not a joke, it is the lesson. `std::vector` is this class, written by
people who have spent longer on it than you will, with the growth strategy and
the exception guarantees handled. Writing the five operations is a thing you do
so that you can read them, recognise a class that gets them wrong, and understand
why the rule of zero is not laziness.

You will write them for real perhaps twice in a career, when wrapping a C library
handle that cannot be expressed as an existing type.

## What Breaks First

- **A double free at exit.** The generated copy constructor copied a pointer, so
  two objects own one buffer. See `E-MEM-0002`.
- **Self assignment destroys the object.** Copy assignment released its own data
  before reading from the source, which was itself. See `E-MEM-0008`.
- **A moved from object still frees the buffer.** The move took the pointer but
  did not clear the source. See `E-CPP-0010`.

## Ship It

`SampleBuffer` goes into `rc::core` as a reference implementation to read rather
than to use, next to a note saying to reach for `std::vector`. The habit it
leaves is the useful part: when a class owns something directly, decide all five,
and when it does not, write none of them.
