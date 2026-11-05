# How an Object Starts: Constructors, the Initialiser List, and the Word explicit

> Setting two members in the constructor's body built each of them blank and
> then overwrote it: two builds, two assignments, for work the list does in two.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-01, 02-03, 02-05

## The Problem

Lesson 01-13 wrote a constructor. Lesson 02-03 attached cleanup to one: acquire
in the constructor, release in the destructor. Lesson 03-01 is about classes, and
the word does not appear in it.

So a constructor has been in the code for a long time and nobody has said what it
is, what the odd line after the colon is, or why some constructors begin with
`explicit`. This lesson is that page. It sits in phase 02 and not phase 03
because 02-03 cannot be understood without it, and an ordering oddity is better
admitted than hidden.

A **constructor** is the function that runs when an object is made, before
anything else can touch it. Its job is to make the object true: every member
holding a value, and every promise the class makes holding from the first
moment.

```cpp
class Calibration {
 public:
  Calibration(const Tracked& offset, const Tracked& scale)
      : offset_(offset), scale_(scale) {}      // the list, then the body
```

The part after the colon is the **member initialiser list**. The braces are the
body, here empty.

## The Concept

### The list builds, the body assigns

By the time the body runs, **every member already exists**. A member you did not
mention in the list was built blank, and an assignment in the body then
overwrites it.

Two members of a type that counts its own construction, set both ways:

```
                          built blank   built as a copy   overwritten
    in the list                     0                 2             0
    in the body                     2                 0             2
```

The same result with twice the work. For an `int` the extra step costs nothing.
For a `std::string` or a `std::vector` it is an allocation made and thrown away,
and for a type that has no blank form it does not compile at all. The suite holds
your `Calibration` to the first row.

`E-CPP-0067`.

### The order is the declaration's, not the list's

```
  the list was written second_, first_. built, in order: first second
```

Members are built in the order they are **declared in the class**, whatever order
the list is written in. gcc 11 and clang 14 both warn about a list that
disagrees, as `-Wreorder`, and both are right. The list is saying something that
is not what happens, and when one member's initialiser uses another it reads that
member before it exists.

Write the list in declaration order, and treat the warning as the finding it is.

### A constructor with one value is also a conversion

```cpp
struct Loose { Loose(double v); };
int whole_part(Loose loose);

whole_part(7);        // compiles: the 7 became a Loose, unasked
```

Measured: it compiled and returned 7. A constructor that can be called with one
value is, unless it says otherwise, a way for that value to turn into the class
without anyone writing a call. Every function that takes a `Loose` then accepts
every number in the program.

`explicit` removes that:

```
  is a double convertible to Loose   yes
  is a double convertible to Meters  no
  can a Meters be made from a double yes
```

You can still make a `Meters` from a double, by saying so, `Meters(5.0)`. You
cannot get one by accident. `Meters m = 5.0;` is refused by every compiler, which
`checks/a_constructor_that_was_explicit.cpp` proves, and so is this line from the
standard library:

```cpp
std::vector<int> v = 5;      // refused: the size constructor is explicit
```

because a vector equal to 5 is not something anybody means. Lesson 03-06 puts
this to work: a unit system is a class with an `explicit` constructor, and the
compiler refusing a bare number is the whole mechanism.

`E-CPP-0066`.

### Declaring a constructor takes the default one away

```
  default constructible, when the class declares:
    only a constructor taking an int      no
    that, and = default                   yes
    Meters, which takes one value         no
    Ramp, which asks for it back          yes
```

If you write any constructor, the compiler stops writing the no-argument one,
reasoning that you have said how the object is made. `Meters m;` then fails:

```
error: no matching function for call to 'Meters::Meters()'          (gcc)
error: no matching constructor for initialization of 'Meters'        (clang)
```

That is often the right design. A distance nobody measured is not zero, it is
nothing, and a default of zero would be a lie that looks like a reading.
`checks/a_class_with_no_default.cpp` is that refusal.

When "has not been given a value" **is** a meaningful state, ask for the default
back with `= default`, and give every member its value where it is declared:

```cpp
class Ramp {
 public:
  Ramp() = default;
  explicit Ramp(double target) : target_(target) {}
  Ramp(double target, double step) : target_(target), step_(step) {}
 private:
  double current_ = 0.0;
  double target_  = 0.0;
  double step_    = 1.0;
};
```

Each constructor starts from the defaults and says only what it changes. That is
also the answer to lesson 02-09: a defaulted constructor on a class whose members
have no values leaves them uninitialised, and these members all have one.

`E-CPP-0065`.

### The rule

> Initialise in the list, in declaration order. Mark a one-value constructor
> `explicit` unless the conversion is the whole point. Give a class a default
> constructor only when "not yet given a value" is a state that makes sense.

## Build It

Three classes.

`Calibration` subtracts an offset and applies a scale. Its constructor builds
both members in the list, and the suite counts: no member may be built blank, and
the copies must be of the offset and then the scale, which is declaration order.

`Meters` is a distance. Its one constructor is `explicit`, it has no default, and
`to_feet` multiplies by 3.28084. The suite asks the compiler, with the standard
library's type traits, whether a bare double converts to it, because a
conversion that does not happen cannot be observed by calling anything.

`Ramp` moves toward a target one step at a time and never past it. It gives each
member its default where it is declared, has an `explicit` constructor from the
target, and one from the target and step.

The stubs are wrong in ways that compile: the constructor assigns in the body, the
constructor is not `explicit`, and the ramp starts at 99. The suite checks the
ramp's starting state as well as the traits, because a class that merely has a
default constructor would otherwise pass for one that starts in the right place.

## Use It

```cpp
const Calibration calibration(offset, scale);
const Meters clearance(0.45);
Ramp ramp(10.0, 0.5);
```

You will meet these everywhere from here on: `librc` classes take their values in
the list, and the ones that wrap a number are `explicit`.

Two limits.

**This does not make a constructor safe.** A constructor can still leave an
invariant false, or throw halfway through, and what happens to the members
already built is lesson 02-03's destructors doing their work.

**The copy and move constructors are not covered.** Lesson 02-05 is those, and
`Tracked` counting its copies is how the table above was made.

## What Breaks First

`E-CPP-0065` is the line that stopped compiling because declaring a constructor
removed the default one, and the baffling version of it from inside
`std::vector<T> v(10)`.

`E-CPP-0066` is the number that became an object by itself, and the oblique error
when it will not.

`E-CPP-0067` is the `-Wreorder` warning that is not pedantry, and the constructor
doing every member's work twice.

## Ship It

No header graduates from this lesson. Its output is three habits, the three in
the rule above.

From here on, a `librc` class that wraps one number has an `explicit` constructor,
and a constructor sets its members in the list.
