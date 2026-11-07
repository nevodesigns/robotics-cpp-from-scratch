# The Function That Did Not Override: One Missing const, a Virtual Call in a Constructor and a Copy That Forgot What It Was

> A class hierarchy is the one place where a small typo changes which code runs and nothing says so.
> Three of them, each measured, and the one word that catches the first.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-05, 03-10

## The Problem

Lesson 03-05 introduced `virtual`: a base class declares what every kind can do, each kind does it its own way, and the caller holds the base. Lesson 03-10 added the destructor that must be virtual too.

What neither showed is how quietly the arrangement can fail. A controller is written, passes its own test, and does nothing in the robot. A base class announces itself and gets the wrong answer. A function that is handed a controller behaves as if it were handed nothing. None of these gives a warning
by default.

## The Concept

### The base class

```cpp
class Controller {
 public:
  virtual ~Controller() = default;
  virtual double output(double error) const { return 0.0; }
  virtual const char* name() const { return "none"; }
};
```

The base version does nothing, so a controller nobody has written yet is a safe one. `Proportional` gives `gain * error` and `Saturated` wraps another controller and holds its output to plus or minus a limit. A wrapper takes a `const Controller&`, so it can wrap
a proportional controller or another wrapper, and the wrapped one must outlive it.

### A missing const

`double output(double error)` and `double output(double error) const` are different functions. A derived class that leaves off the `const` the base has does not replace the base's function, it adds another next to it.

| called | gain 5, error 2 |
|---|---|
| on the `Forgetful` itself | 10 |
| through a reference to `Controller` | 0 |

The tests of the class itself pass, and the robot, which holds it as a `Controller`, gets the do-nothing version.

`override` after the function is a request that this must replace a function of the base, exactly. With it, the same typo is a build error, and the lesson ships a snippet in `checks/` that the build tries to compile and expects to fail:

```
error: 'double Forgetful::output(double)' marked 'override', but does not override
```

See `E-CPP-0091`.

### A virtual call in a constructor

While the base constructor runs the object is a base object, because the derived part is not built yet. A virtual call in it runs the base version. Making a `Special`, the base constructor asked `who()` and was told **"base"**, and asked afterwards it said **"special"**. The advice
is not to call virtual functions in a constructor or a destructor, and to pass what the base needs as an argument instead. See `E-CPP-0092`.

### A copy that forgot what it was

```cpp
double run_by_value(Controller c, double error) { return c.output(error); }
```

A parameter of the base type is a new base object, copied from the base part of the argument. Gain 3, error 2: by reference **6**, by value **0**. This is slicing. A polymorphic type goes by reference or pointer. See `E-CPP-0093`.

## Build It

`Proportional` and `Saturated` in `exercise/solution.hpp`, each with `override` on both functions. The base class and the two `run_by` functions are given.

```
rcpp verify 03-14
```

## Use It

`rc/control/controller.hpp` joins `rc::control`.

```cpp
Proportional p(10.0);
Saturated s(p, 1.0);
const Controller& c = s;
double u = c.output(error);
```

**What this does not do.** It has no state: a controller with memory, like an integral, needs `output` to be non-const or to keep `mutable` state, and that is a design decision lesson 14-01 makes for the PID. `Saturated` keeps a reference, so if the
controller it wraps is destroyed first, it dangles (lesson 02-07). `override` catches a mistaken signature and not a mistaken idea. And virtual calls cost an indirection that a template would not, which rarely matters at control-loop rates and sometimes does.

## What Breaks First

- **A missing const.** The base version runs. See `E-CPP-0091`.
- **A virtual call in a constructor.** The base version runs. See `E-CPP-0092`.
- **A controller passed by value.** Sliced to the base. See `E-CPP-0093`.

## Ship It

`rc/control/controller.hpp` holds `Controller`, `Proportional`, `Saturated` and the two `run_by` functions.

Lesson 03-05 gave the reason to have a base class. This is the checklist for having one safely: `override` everywhere, nothing virtual in a constructor, and references.
