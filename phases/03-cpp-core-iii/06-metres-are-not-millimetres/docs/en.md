# Metres Are Not Millimetres: Making the Compiler Check the Units

> Everything a type system promises is a thing that does not compile, and there
> is no assertion for that. So the build compiles it and expects to fail.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 03-05, 01-03

## The Problem

A robot moves a thousand times too far and stops against something. An angle
comes out at fifty-seven times what it should be. A timeout fires immediately.

Nothing warned about any of it, because every value was a `double` and every
operation was valid arithmetic.

Metres and millimetres are both doubles. So are radians and degrees, seconds and
milliseconds, a position and a velocity, and a value in the world frame and the
same value in the robot's.

## The Concept

### A tag that exists only to be different

```cpp
template <class Tag>
class Quantity {
 public:
  explicit constexpr Quantity(double value) : value_(value) {}
  constexpr Quantity operator+(Quantity other) const { ... }
  // and nothing that takes a different Tag
 private:
  double value_ = 0.0;
};

struct MetreTag {};
using Metres = Quantity<MetreTag>;
```

The tag has no members and is never instantiated. It exists so that
`Quantity<MetreTag>` and `Quantity<SecondTag>` are different types, and the
arithmetic is defined between two of the same tag and nowhere else.

**It costs nothing.** `sizeof(Metres)` is `sizeof(double)`, it is trivially
copyable, and every operation is a function that disappears. There is no runtime
representation of a unit anywhere in the program.

### What the operations return

| expression | result |
|---|---|
| `quantity + quantity` | a quantity |
| `quantity * number` | a quantity |
| `quantity / number` | a quantity |
| **`quantity / quantity`** | **a plain number** |
| `quantity * quantity` | does not compile |

The fourth row is the interesting one. A length divided by a length is a ratio,
which has no unit, so it returns a `double`. That is not a special case bolted
on; it is what the arithmetic means.

And the fifth is deliberate. A length times a length is an area, which this
system has no name for, so it is left undefined rather than silently producing a
length.

### The conversion lives in one place

```cpp
constexpr Metres millimetres(double value) { return Metres(value / 1000.0); }
constexpr double as_millimetres(Metres length) { return length.value() * 1000.0; }
```

This is where a unit system earns its keep. Not in the arithmetic, which was
probably right anyway, but in **a factor of a thousand existing in exactly two
lines of the program** instead of wherever somebody remembered it.

And the constructor is `explicit`, so a bare number cannot become a quantity by
being passed to something that wanted one. The conversion has to be written
down, at the edge, where somebody decided what the number meant.

### Testing what does not compile

Here is the difficulty that makes this lesson different.

Everything a type system promises is a thing that **does not** compile. A test
can only assert what runs, so the guarantee ends up asserted in a comment, and a
later change quietly removes it: somebody adds an implicit conversion for
convenience, or defines `operator*` between two quantities because one call site
wanted it, and every test still passes.

So this lesson ships three programs that must not compile:

```
checks/adding_metres_to_seconds.cpp
checks/a_bare_double_becoming_a_length.cpp
checks/multiplying_two_lengths.cpp
```

`rc_add_lesson` builds a target for each, excluded from the default build, and
registers a test that runs that build and is marked `WILL_FAIL`. **A snippet
that starts compiling is a test failure**, reported like any other, on every
toolchain, on every push.

Three details that matter.

**Compile them against the finished implementation.** A half-written type fails
to compile for reasons that have nothing to do with the guarantee, and the check
would pass for the wrong reason.

**One refusal per snippet.** A file that fails for two reasons still passes when
one of them stops working.

**Say at the top what it is proving**, because a file whose entire purpose is to
fail looks like a mistake to the next reader.

The mechanism is not specific to units. Every promise a compiler makes can be
checked this way: a deleted copy constructor, a template that rejects the wrong
argument, a `static_assert` that should fire.

### What it does not catch

```cpp
const Metres height = metres(1.8);
const Metres wavelength = metres(0.0000005);
const Metres meaningless = height + wavelength;   // compiles, and it should
```

Both are lengths in metres. There is nothing for a unit system to object to.

The same goes for everything that is not a unit. **Frames**: a position in the
world and one in the robot's body are both metres. **Ranges**:
`degrees(370.0)` is a perfectly good `Radians`, and nothing here wraps it,
because wrapping is a decision about meaning. **Sign conventions**, and
**sensibleness**: a distance of a light year is a `Metres`.

So be precise about what was bought. It removes one class of mistake completely,
at zero runtime cost, and that class was a large share of the numeric faults in
this curriculum's atlas. Everything else still needs the ordinary tools.

And more tags are cheap. If frame confusion is a real risk in a particular
program, `Quantity<WorldMetreTag>` and `Quantity<BodyMetreTag>` are two more
empty structs and one conversion function that has to be called: exactly the
shape of the fix that worked for units.

**A type can only enforce a distinction you actually made in the type.** Deciding
which distinctions are worth making is design, and it is the part the compiler
cannot do.

## Build It

Implement the constructor, the arithmetic and the conversions in
`exercise/solution.hpp`.

```
rcpp verify 03-06
```

The suite checks the size and the arithmetic, converts in both directions, and
then the build compiles three programs that must not compile and reports a
failure if any of them does.

## Use It

**Give a unit type to anything that has been confused once.** Distance and time
first, angle second.

**Put every conversion in a named function**, and let the call site say the
unit.

**Keep the constructor explicit.**

**Write a `checks/` snippet for every refusal you rely on**, and one per file.

## What Breaks First

- **A number that does not say what it is.** See `E-NUM-0017`.
- **A guarantee that only exists in a comment.** See `E-NUM-0018`.
- **Mistaking a unit for a meaning.** See `E-NUM-0019`.

## Ship It

`Quantity`, the three tags and the conversions join `rc::core` as
`rc/core/units.hpp`. And `rc_add_lesson` gained a `checks/` directory, so every
later lesson that relies on the compiler refusing something can prove it does.
