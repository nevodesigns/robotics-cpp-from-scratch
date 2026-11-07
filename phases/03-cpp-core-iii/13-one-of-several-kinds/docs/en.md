# One of Several Kinds: A Command That Can Only Be What It Says, and a Compiler That Checks You Handled Them All

> A robot is sent three kinds of command and each needs different things. A type that is exactly one of
> them cannot carry nonsense, and the compiler can check that every kind has been dealt with.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-05, 01-06, 03-01

## The Problem

A robot is sent commands: stop, drive with two wheel speeds, set a speed limit. The obvious type is one struct with a number saying which kind it is and a field for everything any kind needs:

```cpp
struct Command { int kind; double left; double right; double max_speed; };
```

It can hold a stop with wheel speeds left in it, a drive whose speeds were never set, and a kind that is none of the three. Nothing says which fields belong to which kind, so every function that reads one must remember. And when a
fourth kind is added, every `switch` over `kind` that has no case for it builds and does nothing.

Lesson 01-06 gave a state its own type. This is the same idea for things that carry different data.

## The Concept

### A type that is exactly one of several

```cpp
struct Stop {};
struct Drive { double left; double right; };
struct SetLimit { double max_speed; };
using Command = std::variant<Stop, Drive, SetLimit>;
```

A `std::variant` holds exactly one of its alternatives at a time, with only that alternative's data. A stop has no wheel speeds to leave behind, and a drive cannot be missing them.

Measured: a default-constructed `Command` held the first alternative, `Stop` (index 0), which is why `Stop` has to be constructible without arguments. After `c = Drive{1.0, 2.0}` its index was 1. See `E-CPP-0088`.

### Asking what it holds

- `std::holds_alternative<Drive>(c)` says whether it is one.
- `std::get_if<Drive>(&c)` gives a pointer to it, or `nullptr` if it is another kind: a polite question with an answer.
- `std::get<Drive>(c)` gives it or throws `std::bad_variant_access` if it is another kind: asking as if sure.

### One function for each kind

`std::visit` calls the function that matches what the command holds. A visitor is a struct with one `operator()` for each kind:

```cpp
struct Describer {
  std::string operator()(const Stop&) const;
  std::string operator()(const Drive&) const;
  std::string operator()(const SetLimit&) const;
};
std::string describe(const Command& c) { return std::visit(Describer{}, c); }
```

`describe` gives `stop`, `drive 0.5 -0.25` and `limit 1.5`.

### The compiler counts the kinds

Add a fourth kind, `Park`, and visit it with the three-kind `Applier`: the build fails, with `no type named 'type' in 'struct std::invoke_result<Applier, Park&>'`. The lesson ships that snippet in `checks/` and the build tries to compile it and expects
it to fail, so the guarantee is checked and not remembered. A `switch` on a type field would have built. See `E-CPP-0089`.

### A limit applies now

`SetLimit` changes the limit and also clamps the speeds the wheels have now. Driving at 0.8 and -0.4 and then setting a limit of 0.5 left the wheels at **0.5 and -0.4**; the one over the limit came down and the other did not move. A drive of 3 and -3 after
that gave 0.5 and -0.5. See `E-CPP-0090`.

### Reading a line

`parse_command` turns `stop`, `drive 0.25 -0.5` and `limit 2` into the right kind. It refuses ten bad lines: an empty one, an unknown word, `drive 1` with a value missing, `drive a b`, `drive 1 2 3` with one too many, `stop now`, `limit 0`, `limit -1`, `limit` and `limit 1 2`. A refused
line leaves the output as it was.

### What it costs

`sizeof(Drive)` was 16 and `sizeof(Command)` was 24: the largest kind and a tag.

## Build It

The three visitors' functions and `parse_command` in `exercise/solution.hpp`. The kinds, `RobotState` and `clamp_speed` are given.

```
rcpp verify 03-13
```

## Use It

`rc/core/command.hpp` joins `rc::core`.

```cpp
Command c;
if (parse_command(line, &c)) apply(state, c);
```

**What this does not do.** It is for a small, closed set of kinds that one program knows about. A set that plugins add to is a job for a base class with virtual functions (lesson 03-05). It has no way to carry an error from the parser: the caller gets false and no reason, where
lesson 00-10 returned one. A variant whose alternatives can throw while being copied can end up holding nothing, which is rare and worth reading about. And the text format is a toy: no comments, no quoting and no units.

## What Breaks First

- **A kind field and a struct of everything.** Nonsense is representable. See `E-CPP-0088`.
- **A new kind nothing handles.** A `switch` builds and does nothing. See `E-CPP-0089`.
- **A limit that waits for the next command.** See `E-CPP-0090`.

## Ship It

`rc/core/command.hpp` holds `Stop`, `Drive`, `SetLimit`, `Command`, `RobotState`, `describe`, `apply` and `parse_command`.

Lesson 03-02 built `rc::expected` on a `std::variant` and said to read it. This is how to use one directly.
