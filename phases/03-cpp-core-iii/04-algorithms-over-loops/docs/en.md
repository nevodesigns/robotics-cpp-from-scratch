# Algorithms: The Loops You Should Not Write

> A loop tells the reader how. A named algorithm tells them what, and takes the off by one with it.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-03

## The Problem

Here is a loop that appears in every robotics codebase, in some form, several
times.

```cpp
int count = 0;
for (int i = 0; i <= readings.size(); ++i) {
  if (readings[i].celsius > limit) count++;
}
```

It has a bug you have already met, an index running one past the end. It is also
six lines that say almost nothing about intent: a reader has to execute it in
their head to work out that it counts readings above a limit.

```cpp
const auto count = std::count_if(readings.begin(), readings.end(),
                                 [limit](const Reading& r) { return r.celsius > limit; });
```

One line, the intent is the function's name, and the bounds are not yours to get
wrong. This lesson is the handful of algorithms that cover most of what a robot
does to a batch of readings, and the three traps that come with them.

## The Concept

### Ranges are a pair, and the end is one past the last

Every algorithm takes a beginning and an end, and the end points **one past** the
last element. `readings.end()` is not the last reading; it is the position after
it. That is why a loop to `size()` inclusive is wrong and why
`begin()` to `end()` is right.

An algorithm that searches returns `end()` to mean not found, for the same reason
`nullptr` means no such thing: it is a value that cannot collide with a real
answer.

```cpp
const auto found = std::find_if(readings.begin(), readings.end(), is_hot);
if (found == readings.end()) return std::nullopt;
return *found;
```

The comparison against `end()` is compulsory. Dereferencing `end()` is undefined
behaviour, and it is the same mistake as dereferencing a null pointer.

### The six worth knowing today

| Algorithm | Answers |
|---|---|
| `std::find_if` | where is the first one that matches |
| `std::count_if` | how many match |
| `std::accumulate` | fold the whole range into one value |
| `std::transform` | make a new range from this one |
| `std::sort` | put them in order |
| `std::remove_if` with `erase` | drop the ones that match |

That is most of what happens to a batch of sensor data. Everything else is these
combined.

### Trap one: remove does not remove

This is the strangest interface in the standard library and it catches everyone
once.

`std::remove_if` cannot change the size of a container, because it only has
iterators and iterators cannot resize anything. What it does is shuffle the
elements you are keeping to the front and return an iterator to the new end. The
container is still the same length, with unspecified values in the tail.

So removing is two steps, and the idiom has a name:

```cpp
readings.erase(std::remove_if(readings.begin(), readings.end(), is_invalid),
               readings.end());
```

Call `remove_if` alone and the size never changes, the count is wrong, and there
is no error of any kind. C++20 added `std::erase_if`, which does both and is what
you should use the day the project moves to it. In C++17 the two step idiom is
the way, and knowing why is worth more than knowing the incantation.

### Trap two: accumulate takes its type from the starting value

```cpp
std::accumulate(readings.begin(), readings.end(), 0)        // adds into an int
std::accumulate(readings.begin(), readings.end(), 0.0)      // adds into a double
```

The difference is one character and it decides the type of every addition. With
`0`, a sum of temperatures like 20.5 and 21.5 truncates each step and reports
41 instead of 42.0. The compiler is content, because integer addition is a
perfectly reasonable thing to ask for.

This is the same family as lesson 00-03. The arithmetic is doing exactly what
you wrote, and what you wrote is not what you meant.

### Trap three: sort needs a strict weak ordering

`std::sort` requires that the comparator answers **strictly** less than. In
particular it must report false when both arguments are the same element:

```cpp
bool earlier(const Reading& a, const Reading& b) { return a.at_ms < b.at_ms; }   // correct
bool earlier(const Reading& a, const Reading& b) { return a.at_ms <= b.at_ms; }  // broken
```

With `<=`, an element counts as before itself, and the sort searching for a pivot
walks past the end of the range looking for something that cannot exist. The
result is a heap buffer overflow inside the standard library, which is the crash
you triaged in lesson 04-02: nine frames of template noise, and the only line
anybody wrote is the call to `sort`.

The rule is short. A comparator must be false for equal elements, and if you find
yourself writing `<=` in one, stop.

## Build It

`exercise/solution.hpp` gives you `Reading`. Implement each of these with an
algorithm rather than a hand written loop:

- `first_above(readings, limit)`, the first reading above a temperature, or
  nothing.
- `count_valid(readings)`, how many are marked valid.
- `mean_celsius(readings)`, the mean, and 0.0 for an empty batch.
- `to_fahrenheit(readings)`, a new vector of converted values, same length.
- `earlier(a, b)`, the comparator, and `sort_by_time(readings)` using it.
- `drop_invalid(readings)`, which must actually shrink the vector.

```
rcpp verify 03-04
```

## Use It

C++20 adds the ranges library, which lets these compose without naming
`begin()` and `end()` at every call:

```cpp
const auto hot = readings | std::views::filter(is_hot);
```

It is a genuine improvement and it is not available here, for the reason given in
lesson 01-04: the C++17 form is what teaches you what the pieces are. When your
project moves to C++20 the names you learned still mean the same things.

## What Breaks First

- **remove_if left the size unchanged.** It shuffles, it does not erase. See
  `E-CPP-0018`.
- **A mean that lost its fractional part.** The starting value of accumulate was
  an integer. See `E-NUM-0011`.
- **A heap buffer overflow inside std::sort.** The comparator used `<=`, so an
  element compares as before itself. See `E-CPP-0019`.

## Ship It

These join `rc::core` as the batch helpers every later phase uses on a window of
readings. The habit is the real artifact: reach for the named algorithm, and the
bounds stop being yours to get wrong.
