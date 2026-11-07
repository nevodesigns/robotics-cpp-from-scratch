# Which Function Got Called: A String Literal That Chose bool, and a Default Argument That Came From the Wrong Class

> The compiler picks a function from the arguments, and the rules favour conversions that cost nothing over ones
> that mean what you wrote. Three cases where the function that runs is not the one that was intended.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-05, 03-14

## The Problem

A function is overloaded: one version for a yes or no, one for a message. It is called with a message and says it went fine. A derived class has a different default from its base, and the default it gets is the base's. A function is called with `0` to mean "no pointer"
and the integer version runs.

No warning, no crash, and the right function was right there. Which function runs follows rules that put a built-in conversion above a user-defined one, and the static type above the dynamic one, and neither is guessable from reading the call.

## The Concept

### A string literal chooses bool

```cpp
void report(bool ok);
void report(const std::string& message);
report("transmission failed");       // which?
```

A string literal is a `const char[N]`. It becomes a `const char*` for free. A pointer becomes a `bool` with one more built-in conversion. Becoming a `std::string` needs a constructor to run, and a built-in conversion ranks above a user-defined one.

The suite measured it: with only those two overloads, the call wrote **"flag: ok;"**, because a pointer to a literal is not null. See `E-CPP-0097`.

The fix is the overload the literal matches exactly:

```cpp
void report(const char* message) { report(std::string(message)); }
```

Then the same call wrote `message: transmission failed;`, and a `std::string` and a `bool` still went to their own.

### A default argument belongs to the reference

```cpp
class BadSink  { virtual std::string send(const std::string&, int retries = 3) const; };
class BadQuiet : public BadSink { std::string send(const std::string&, int retries = 0) const override; };
```

Which function runs is decided when the program runs, from the object. Which default is filled in is decided when it is compiled, from the type of the reference in the call. A quiet sink that defaults to 0 gave:

| called | retries |
|---|---|
| on the `BadQuiet` itself | 0 |
| through a reference to `BadSink` | 3 |

The derived function ran, with the base's number. See `E-CPP-0098`.

The fix is to have no default argument on a virtual function. The default becomes a virtual function of its own:

```cpp
std::string send(const std::string& text) const { return send_with(text, default_retries()); }
virtual int default_retries() const { return 3; }      // Quiet returns 0
```

Through a base reference the quiet sink then gave `x retries=0`, a plain one `x retries=3`, and an explicit `send_with("x", 7)` gave 7.

### Zero is an integer

`which(0)` chose **int** and `which(nullptr)` chose **pointer**. `0` is an `int` that converts to a null pointer, and with an `int` overload present the exact match wins. Write `nullptr` when a null pointer is meant. See `E-CPP-0099`.

## Build It

The `const char*` overload of `Reporter`, `Sink::send` and `Quiet::default_retries` in `exercise/solution.hpp`.

```
rcpp verify 03-17
```

The tests keep the mistaken versions, `Naive` and `BadSink`, and measure what they do, as well as what yours does.

## Use It

`rc/core/reporter.hpp` joins `rc::core`.

```cpp
Reporter r(&log);
r.report("transmission failed");     // a message
r.report(ok);                        // a flag
```

**What this does not do.** It handles `const char*` but not a `char*`, a `std::string_view` or a wide string, each of which has its own conversions. It does not stop a caller passing a pointer to something else by mistake: a pointer to an `int` also becomes a `bool`.
Making the `bool` overload `explicit` in some way, or giving it a different name, such as `report_ok`, is the sturdier design, and the overloads here are there to show how the rules work. And it says nothing of templates, whose deduction is another set of rules.

## What Breaks First

- **A literal and a `bool` overload.** The flag wins. See `E-CPP-0097`.
- **A default argument on a virtual function.** The reference's default. See `E-CPP-0098`.
- **A zero for a null pointer.** The integer overload. See `E-CPP-0099`.

## Ship It

`rc/core/reporter.hpp` holds `Reporter`, `Sink` and `Quiet`.

Lesson 03-14 showed one way a call can reach the wrong function. These are three more, and the habit is the same: say which one is meant.
