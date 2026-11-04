# Making It Iterable: What range-for Needs, and What the Algorithms Need

> A ring of four holding 3 4 5 6 keeps them in its slots as 5 6 3 4. An
> iterator over the array gives you the right four readings in the wrong order,
> and the sum comes out correct either way.

**Type:** Build
**Time:** about 135 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 03-04, 03-07

## The Problem

Lesson 03-07 built a `Ring`, and you cannot write this:

```cpp
for (double reading : ring) { ... }
```

The compiler looks for `begin()` and `end()` and does not find them. Nor can you
pass it to `std::accumulate`, `std::max_element` or anything else in
`<algorithm>`, which lesson 03-04 spent its time persuading you to prefer over
writing loops by hand.

So that advice quietly does not apply to anything you wrote yourself, and every
loop over your own container goes back to being an index and a `for`.

An iterator fixes it. The useful part is what writing one teaches about what the
language is actually asking for, which is much less than people expect in one
place and more than they expect in another.

## The Concept

### range-for asks for three operations

That is the whole requirement. The loop expands to something that
dereferences, pre-increments, and compares for inequality:

```cpp
reference operator*() const;
IndexIterator& operator++();
bool operator!=(const IndexIterator& other) const;
```

Plus a `begin()` and an `end()` to get a pair of them from. Measured: a struct
with `size()`, `operator[]` and exactly those three operations iterated
correctly.

`end()` is one past the last element, which for an index based iterator means
the size itself. There is no element there and there does not need to be,
because `end` is never dereferenced, only compared against.

### The design decision: index, not storage

Here is the measurement that decides how to write it. A ring of four with 1 to
6 pushed into it:

| | order |
|---|---|
| `operator[]` | 3 4 5 6 |
| an iterator over `operator[]` | 3 4 5 6 |
| the raw slots | **5 6 3 4** |

The elements are not in array order. The oldest is wherever the reuse left it,
and `operator[]` exists to do that arithmetic.

So an iterator built on `operator[]` is correct for free, and one built on the
storage is wrong. Which is why `IndexIterator` holds a container pointer and an
index rather than an element pointer.

And now the part that makes this bug survive:

> The sum is right either way.

`std::accumulate` does not depend on order, so the mean is right, the maximum
value is right, and every order independent check passes. What fails is
anything about position: the oldest reading, the newest, a derivative, a plot,
the iterator that `max_element` hands back. And it only fails after the buffer
has wrapped, so a test that pushes fewer readings than the capacity cannot see
it, which is the same blind spot as `E-CPP-0041`.

The check that distinguishes the two implementations is one line: compare the
walked sequence against the `operator[]` sequence.

### The algorithms ask for five typedefs

```cpp
using iterator_category = std::random_access_iterator_tag;
using value_type = typename Container::value_type;
using difference_type = std::ptrdiff_t;
using pointer = const value_type*;
using reference = const value_type&;
```

Nobody guesses those. They are looked up, through
`std::iterator_traits`, and a missing one produces an error from inside
`<algorithm>` rather than from your own file, which is lesson 03-07's
measurement about template diagnostics arriving somewhere unhelpful.

The container needs one too. `value_type` is the first thing a container says
about itself, because anything generic has to be able to **name** the element
type rather than be told it separately. `rc::core::Ring` gained that line for
exactly this reason, and a container without it is in `checks/` as code that
must not compile.

With them in place:

```
accumulate      18
max_element     6
min_element     3
count_if > 4    2
distance        4
```

### The category is a promise, not a label

`iterator_category` does not describe what you wrote. It **selects** which
operations the library will require of you and which implementations it will
choose.

Declare `random_access_iterator_tag` and `std::distance` subtracts instead of
counting, `std::lower_bound` jumps to the middle instead of walking, and
`std::advance` uses `operator+=`.

Which means you owe the whole set. Writing this lesson, omitting `operator+=`
produced:

```
error: no match for 'operator+=' (operand types are
'IndexIterator<Samples>' and 'long int')
```

from `/usr/include/c++/11/bits/stl_iterator_base_funcs.h`, about an operator I
had not realised I had agreed to provide.

So claim the weakest category that is true. `forward_iterator_tag` owes far
less and is enough for `find`, `count_if`, `accumulate` and range-for. For an
index based iterator the random access operations are one line each, because
the work really is an index addition, which is what makes the claim honest.

One detail: the iterator difference returns a signed `difference_type`, and the
indices are unsigned, so cast each one **before** subtracting. Subtracting
first and converting afterwards wraps, which was lesson 01-07.

### Read only, on purpose

The references handed out are `const`, so `std::sort` is refused.

That refusal is the type system saying what the design already decided: a
window of readings you have collected is not a thing to reorder in place. Copy
it into a `std::vector` and sort that, which costs an allocation and says
plainly that a sorted copy is a different object from the history.

It is in `checks/` so the error is met deliberately rather than at an awkward
moment.

### One iterator, any container

The payoff is that none of this is per container. `IndexIterator` asks only for
`operator[]`, `size()` and `value_type`, so the same template served a `Ring`,
a struct defined in the test file, and a `std::vector` which has perfectly good
iterators of its own.

## Build It

Implement `operator*`, `operator++` and `operator!=`, the random access
operations, and `index_begin` and `index_end` in `exercise/solution.hpp`.

```
rcpp verify 03-08
```

The suite walks a wrapped ring and compares the sequence against both the
indexing and the raw slots, runs a range-for over a container of its own,
exercises five algorithms, checks the random access operations the category
promised, and then uses one iterator on three unrelated containers. Two
snippets in `checks/` are compiled and expected to fail.

The stubs you start from deliberately make every range empty, so the suite
fails quickly rather than looping. An `operator++` that does not advance makes
every loop run forever, and a test that hangs tells you nothing.

## Use It

**Give any container you write a `value_type`, a `begin()` and an `end()`.**
Three lines, and it joins the standard library rather than sitting outside it.

**Build the iterator on the indexing, not the storage**, so the arithmetic lives
in one place.

**Claim the weakest category that is true**, and test the operations it
promised.

**Test past the capacity, with an order dependent check.** A sum will not catch
a wrong order.

**Reach for `rc::core::IndexIterator`** rather than writing another one. Any
container with those three members gets everything.

## What Breaks First

- **An iterator over the storage instead of the index.** See `E-CPP-0046`.
- **A container the algorithms will not accept.** See `E-CPP-0047`.
- **A category promised and not kept.** See `E-CPP-0048`.

## Ship It

`IndexIterator`, `index_begin` and `index_end` join `rc::core`, and
`rc/core/ring.hpp` gains `value_type`, `begin()` and `end()`, so the ring buffer
from 03-07 now works with range-for and the whole of `<algorithm>`. Every
windowed statistic in phases 06 and 15, and every pose trail in phase 16, is
written against that rather than against an index.
