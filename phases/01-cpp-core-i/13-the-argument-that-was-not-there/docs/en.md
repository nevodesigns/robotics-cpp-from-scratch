# The Argument That Was Not There: Reading a Command Line Without Lying About It

> The shape this lesson warns about is in this repository's own tool. The
> `flag_value` in `tools/rcpp/main.cpp` returns a fallback, so a user who
> never asked and a user who asked and typed it wrong get the same answer.
> That is where the lesson came from.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-04, 01-05, 01-12

## The Problem

Every program you have written so far has had its inputs written into it. You
changed a number, rebuilt, and ran it again.

A tool is different. It is told its inputs when it is run:

```
plan_path --speed 0.4 route.csv
```

and the telling happens through the two parameters of `main` that nobody
explains.

```cpp
int main(int argc, char** argv)
```

Three things go wrong here, and all three are silent. The program runs, returns
0, and does the wrong thing.

## The Concept

### What argc and argv actually are

`argc` is how many strings there are. `argv` points at them. `argv[0]` is
conventionally the program's own name, so the arguments a user typed start at
`argv[1]` and there are `argc - 1` of them.

For the command line above, the suite prints what that means:

```
  "plan_path --speed 0.4 route.csv" is argc 4, and the
  three arguments the user typed start at argv[1]
```

Two guarantees are worth having before you touch it.

**`argv[argc]` is a null pointer.** The standard requires it. So walking until
null and counting to `argc` are both correct, and in a well formed command
line they agree.

**`argv[0]` is not guaranteed to be there.** A process started through some
`exec` calls can have `argc` of 0, so `argv[0]` in a usage message wants a
guard.

### Trap one: the argument that was not there

```cpp
const std::string path = argv[1];     // nothing checked argc
```

Run with no arguments and `argc` is 1, so `argv[1]` is that guaranteed null.

The instinct is that this read is out of bounds. It is not, and that is what
makes it worse. The read is perfectly valid and returns a null pointer, and a
wild pointer usually crashes where the mistake is while a null that looks like
a value travels.

Constructing a `std::string` from it is undefined behaviour. Measured on this
curriculum's libstdc++, the undefined behaviour you get happens to be
pleasant:

```
terminate called after throwing an instance of 'std::logic_error'
  what():  basic_string::_M_construct null not valid
Aborted (core dumped)
```

That is luck, not a guarantee. Another standard library may read from address
zero, and a `const char*` handed straight to a C API will do whatever that API
does with null.

So `Arguments` stops at `argc` **and** at a null entry, and an out of range
index gives an empty view rather than undefined behaviour. A tool reading its
own arguments should not be able to fall over on a command line the user did
not type.

### Trap two: a conversion that cannot fail

A numeric option arrives as text a person typed. `std::atoi` converts it, and
has no way at all to report that it could not: the return type is `int`, every
`int` is a possible answer, and there is no second channel.

The suite measures it, on five strings a user can actually type:

```
  input      atoi   rc::core::parse_number
  42        42     accepted
  abc       0      refused
  (empty)   0      refused
  3.9       3      accepted
  7abc      7      refused
```

Two of the five are numbers, and `parse_number` accepts exactly those two.
`atoi` gets three wrong, in three different ways.

**`0` for things that are not numbers at all**, which is indistinguishable
from a user who typed 0. For a speed, a gain or a timeout, 0 is a perfectly
legitimate value, so there is nothing to detect.

**`3` for `3.9`**, a real number read as a different one, because `atoi` only
ever reads an integer and stops at the dot without comment.

**`7` for `7abc`**, because it stops at the first character it cannot use and
does not consider the rest your problem.

And on a value too large for an `int` it is not merely wrong, it is undefined.
Measured outside the suite, because the suite will not perform undefined
behaviour: `atoi("99999999999999999999")` returned **-1**, while `strtol` on
the same string returned `LONG_MAX` and set `errno` to `ERANGE`.

A mistyped speed limit became a negative speed.

`parse_number` from lesson 01-12 is already in `librc` and already handles
this, including the `1,5` case that `std::stod` reads as `1`. Do not write
another one.

### Trap three: a default that stood in for three answers

This is the one that causes arguments between people, and it is the shape
nearly everyone writes first:

```cpp
std::string flag_value(const Args& args, const std::string& flag,
                       const std::string& fallback);
```

One return channel, four situations:

| what the user did | what comes back |
| --- | --- |
| did not mention the option | the fallback |
| misspelled the option | the fallback |
| gave the option with nothing after it | the fallback |
| gave the option properly | their value |

The first is someone who does not care, and the default is the right answer
for them. The second and third are someone who cares and typed it wrong, and
the default is the wrong answer: they should be told.

No body, however careful, can tell them. The information is destroyed at the
`return`.

So the lookup returns the outcomes instead:

```cpp
struct Option {
  bool present = false;    // the name appeared
  bool has_value = false;  // and something followed it
  std::string_view value;
};
```

and the suite reads the same four lookups both ways:

```
  option            present  has value  value
  --gain            no       no
  --speed           yes      yes        --limit=
  --limit=          yes      yes        (empty)
  --mode fast       yes      yes        fast

  the same four through a fallback of "0.2":
    --gain  0.2
    --limit (empty)
```

For numbers the same thing, with the parse as a third outcome:

```
  option     present  parsed  value
  --speed    yes      yes     0.4
  --gain     yes      no
  --trim     yes      no
  --offset   no       no
```

Three different failures and one success. A tool that only had a `double`
would drive at 0 for three of those rows and could not say which.

### The conversions that are not there

`Option` has no conversion to `bool`. `Arguments::Number` has no conversion to
`double`. Both refusals are in `checks/`, and they are the only thing stopping
the three answers from collapsing again on the first tired afternoon.

If `Option` converted to `bool`, somebody would have to decide what
`if (args.option("--mode"))` meant. Present? Present with a value? Present
with a non-empty value? Each is reasonable, two are wrong at any given call
site, and the choice would be invisible in the code.

If `Number` converted to `double`, the value standing in for "you did not tell
me" would be 0, and 0 is a speed.

### What this does not catch

A misspelled option name. `--sped 0.9` is simply an option the tool does not
have, and no return type knows what the user meant.

Two cheap things do help. **Reject unrecognised arguments that start with a
dash**, so a tool that would silently ignore `--sped` fails instead. And
**print the resolved settings before the work starts**, which turns an argument
about what was typed into something anybody can read off the log.

### Two conventions worth following rather than inventing

**`--name value` and `--name=value` both work.** Users type both. In the
separate form, whatever follows is taken as the value even if it looks like
another option, which is what `getopt` does and therefore what users expect.
The cost is that `--speed --verbose` makes `"--verbose"` the speed, and the
suite measures that case: `number()` reports it will not parse, so the user is
told. A string option has no such safety net, which is the argument for
writing `--name=value` in scripts.

**A bare `--` ends the options.** Everything after it is positional even if it
starts with a dash, which is the only way to open a file called `--help` with
a program that has a `--help`.

**A repeated option takes the last one**, which is what makes a wrapper script
possible: the script supplies defaults and whatever the user adds afterwards
wins.

## Build It

Three pieces, and a test that needs no command line.

Read `main`'s parameters in the constructor. Guard a null `argv` and a
non-positive `argc`, take `argv[0]` as the program name when it is there, then
collect from `argv[1]`, stopping at `argc` or at a null entry, whichever comes
first.

Write `option`, handling both spellings, stopping at a bare `--`, and not
stopping at the first match so a repeated option takes the last.

Write `number` in terms of `option` and `rc::core::parse_number`.

The suite builds its own `argv` by hand, with the trailing null the standard
requires, because running a tool to test its argument handling is slow,
awkward and platform specific, while the thing `main` receives is only an
array of pointers.

## Use It

```cpp
#include <rc/core/args.hpp>

int main(int argc, char** argv) {
  const rc::core::Arguments args(argc, argv);

  const rc::core::Arguments::Number speed = args.number("--speed");
  if (!speed.present) return usage(args.program(), "--speed is required");
  if (!speed.ok)      return usage(args.program(), "--speed needs a number");

  const std::vector<std::string_view> files = args.positionals();
  if (files.empty()) return usage(args.program(), "no input file");

  return drive(speed.value, files[0]);
}
```

Read the flags first and the value second, every time.

## What Breaks First

`E-CPP-0055` is the argument that was not there, and why a guaranteed null is
worse than an out of bounds read.

`E-NUM-0023` is `atoi`, with the five input table and the 20 digit number that
came back as -1.

`E-CPP-0056` is the default that stood in for three answers, which quotes this
repository's own tool as the example.

## Ship It

`rc/core/args.hpp`, in `namespace rc::core`.

`tools/rcpp/main.cpp` still has the hand-rolled version. That is the next
thing to change, and it is the same signal that produced `rc::test::Restore`
in lesson 05-06: when the repository has written something twice by hand, it
belongs in the library.
