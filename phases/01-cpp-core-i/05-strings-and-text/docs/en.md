# Strings: Text That Owns Itself, and Views That Do Not

> A string is a vector of characters that cleans up after itself. A string view is a span of characters that does not. Everything else follows from that one sentence.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-04

## The Problem

Robots are full of text, and none of it is decoration.

A configuration file says `max_speed = 1.5`. A device announces itself as
`ttyUSB0`. A status line reports `imu.temp 41.5 ok`. A log records what happened
so somebody can work out why the machine stopped at three in the morning.

All of it arrives as characters and has to be cut into pieces, compared, and put
back together. Doing that with raw character arrays is where C programs
traditionally went wrong, in ways that had names: buffer overruns, missing
terminators, off by one.

C++ gives you two types for it, and the whole subject is knowing which one you
are holding.

## The Concept

### std::string owns its characters

`std::string` is a growable sequence of characters that manages its own memory.
It is `std::vector` for text, and it behaves the same way: copying it copies the
characters, it grows when you append, and it frees itself when it goes out of
scope.

```cpp
std::string name = "imu.temp";
name += ".raw";                 // now imu.temp.raw
const std::size_t dot = name.find('.');
```

Everything that searches returns a position, and everything that fails to find
returns `std::string::npos`. That is the same idea as `nullptr` and as
`end()`: a value that cannot be confused with a real answer.

**`npos` must be checked before it is used.** It is the largest possible size,
so using it as a position reads far past the end of the text.

```cpp
const std::size_t at = line.find('=');
if (at == std::string::npos) return;   // compulsory
const std::string key = line.substr(0, at);
```

### std::string_view does not own anything

`std::string_view` is a pointer and a length. It is exactly `rc::span` from
lesson 01-04, specialised for characters, and it carries the same warning: it
keeps nothing alive.

It is the right parameter type for a function that only reads text, because it
accepts a `std::string`, a string literal, or a slice of either, and copies
none of them:

```cpp
std::size_t count_char(std::string_view text, char wanted);

count_char(name, '.');          // no copy
count_char("imu.temp", '.');    // no copy, no temporary string
```

The danger is the same as a dangling span, and it has one shape that catches
everybody:

```cpp
std::string_view bad = std::string("temporary");   // the string dies at the semicolon
```

The temporary is destroyed at the end of that statement, and the view now points
at released memory. **Never return a view of a local, and never store one whose
source you do not control.**

### Comparing and building

Comparison is what you expect, and it compares characters rather than addresses,
which is the thing that trips people arriving from C:

```cpp
name == "imu.temp"     // compares the text
```

Building text from pieces is `+` or `+=`, and for anything with values in it,
`rc::format` from the compatibility header reads better than a chain of
concatenations.

### What C++17 does not have

`starts_with` and `ends_with` arrived in C++20. In C++17 they are two lines each,
and writing them is worth doing once, because they show that a prefix test is
just a length check and a compare:

```cpp
bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}
```

When the project moves to C++20, delete them. This is the same bridge as
`rc::span`: you understand the standard version on the day you meet it because
you already wrote it.

## Build It

Implement in `exercise/solution.hpp`. These are the helpers the configuration
parser and the sensor log both use, so you are writing them before they are
needed rather than after.

- `trim(text)` removes spaces and tabs from both ends.
- `starts_with(text, prefix)` and `ends_with(text, suffix)`.
- `split(text, separator)` cuts on a character and returns the pieces. Two
  separators in a row produce an empty piece, which is what a parser wants to
  see rather than have hidden.
- `join(parts, separator)` puts them back, with no trailing separator.
- `count_char(text, wanted)` counts occurrences.

Every parameter that only reads text takes a `std::string_view`. Every return
that hands text back is a `std::string`, because the caller has to own it.

```
rcpp verify 01-05
```

## Use It

That last rule is the whole design in one line: **take a view, return a string.**
A function taking a view copies nothing at the call site, and a function
returning a string leaves no question about who owns the result.

Text handling is where a great deal of real robot code spends its time, in
configuration, in logging, and in device discovery. It is also where the sharpest
tools are worth having, because the failures are silent: a missing check against
`npos` reads memory it should not, and nothing says a word.

## What Breaks First

- **A position used without being checked against npos.** A failed find returns
  the largest possible size, and using it as an index reads far past the end. See
  `E-CPP-0020`.
- **A view of a temporary string.** The string dies at the semicolon and the view
  is left pointing at released memory. See `E-MEM-0001`.
- **substr with a length past the end.** The two argument form takes a position
  and a count, not two positions, which is a mistake worth making once. See
  `E-CPP-0007`.

## Ship It

These join `rc::core`, and lesson 03-02's configuration parser uses `trim` from
here rather than defining its own. The rule goes with them: take a view, return
a string.
