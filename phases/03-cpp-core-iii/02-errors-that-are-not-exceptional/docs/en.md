# Errors That Are Not Exceptional

> A serial port that will not open is not an exceptional circumstance. It is Tuesday.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-01

## The Problem

A robot reads a configuration file at startup. Somebody has typed
`max_speed = fast`.

What should the parsing function do? Four answers are common and three of them
are bad.

Return a default of 1.0 and carry on, so the robot silently runs at a speed
nobody chose. Return a bool and write the answer into an output parameter, which
callers forget to check. Throw, which is fine at startup and forbidden in the
control loop this configuration is about to feed. Or print an error and call
`exit`, taking the whole machine down for a typo.

The fourth answer, and the right one, is to return something that is either the
setting or the reason there is no setting, in a form the caller cannot
accidentally ignore.

## The Concept

### Three kinds of failure, and they are not the same

Almost all confusion about error handling comes from treating these as one
thing:

| Kind | Example | Answer |
|---|---|---|
| **A bug** | index past the end of an array, null where null is impossible | Assert. Crash loudly in testing. Do not handle it, fix it. |
| **An expected failure** | file missing, port busy, bad input, no reading yet | Return it as a value. This is most of them. |
| **Genuinely exceptional** | out of memory, a constructor that cannot build a valid object | Throw. Rare. |

The middle row is where robot code lives, and returning failures as values is
what this lesson is about.

### A value or nothing: std::optional

C++17's `std::optional<T>` holds a value or holds nothing:

```cpp
std::optional<double> find_setting(const std::vector<Setting>& settings,
                                   const std::string& key);
```

The caller cannot get at the value without acknowledging it might be absent:

```cpp
if (const std::optional<double> speed = find_setting(settings, "max_speed")) {
  use(*speed);
}
// or, when a default is genuinely correct:
const double speed = find_setting(settings, "max_speed").value_or(1.0);
```

This is strictly better than returning a sentinel like minus one or NaN, because
there is no in-band value to collide with a real answer, and better than an
output parameter, because ignoring it is not possible by accident.

### A value or a reason: expected

`std::optional` says something is missing. It does not say why, and for a parser
the why is the whole point: the operator needs to know it was a bad number rather
than an unknown key.

`std::expected<T, E>` holds either a value or an error. It arrived in C++23, and
this curriculum is written in C++17, so `rc/core/compat.hpp` provides
`rc::expected` with the same shape. On a newer toolchain it is an alias for the
standard type and your code does not change.

```cpp
rc::expected<Setting, ConfigError> parse_setting(const std::string& line);

const auto parsed = parse_setting(line);
if (parsed) {
  apply(parsed.value());
} else {
  report(parsed.error(), line);
}
```

Open `librc/include/rc/core/compat.hpp` and read it. It is a `std::variant` and
about forty lines, which is genuinely all it is.

### Why robotics leans this way

Exceptions are a good mechanism and this is not an argument against them. But
two properties matter here.

Throwing allocates and unwinds, and neither has a bound you can state. A control
loop with a deadline of one millisecond cannot accept an operation whose worst
case is unknown, so many robotics and embedded codebases build with exceptions
disabled entirely, and some safety standards effectively require it.

Second, an expected failure that travels as a thrown exception tends to be caught
far from where it happened, by a handler that has lost the context needed to do
anything sensible. A returned error is handled by the caller, who still knows
what was being attempted.

The practical rule used from here on in this curriculum: **failures that a
correct program will meet in normal operation are returned as values. Exceptions
are for the genuinely exceptional, above all a constructor that cannot produce a
valid object.**

### Parsing without throwing

`std::stod` throws on bad input, which defeats the purpose. A stream does not:

```cpp
std::istringstream in(text);
double value = 0.0;
in >> value;
if (!in || !in.eof()) return rc::unexpected(ConfigError::NotANumber);
```

Two checks, and both are needed. `!in` catches text that is not a number at all.
`!in.eof()` catches text that starts with a number and continues, such as
`1.5kg`, which otherwise parses as 1.5 and silently drops the rest.

## Build It

Implement in `exercise/solution.hpp` a parser for lines like `max_speed = 1.5`:

- `parse_setting(const std::string& line)` returns
  `rc::expected<Setting, ConfigError>`, with these errors in this order of
  checking: `Empty` for a blank or whitespace only line, `MissingEquals` when
  there is no `=`, `UnknownKey` when the key is not one of `max_speed`,
  `wheel_base` or `loop_hz`, `NotANumber` when the value does not parse, and
  `OutOfRange` when the number is not greater than zero.
- Whitespace around the key and the value is ignored.
- `find_setting(settings, key)` returns `std::optional<double>`.
- `describe(ConfigError)` returns a message an operator can act on.

```
rcpp verify 03-02
```

## Use It

`std::optional` is in C++17 and is available everywhere. Use it whenever a
function may legitimately have no answer.

`std::expected` is C++23. Until a project is on it, `rc::expected` here, `tl::expected`
and `absl::StatusOr` are the same idea, and error code enumerations with an
output parameter are the older form you will meet in every C library and in ROS 2
service results.

## What Breaks First

- **You called value() on an error, or error() on a value.** Ask which it holds
  first. See `E-CPP-0012`.
- **A number parsed as valid when it was not.** You checked the stream state but
  not that it consumed the whole text, so `1.5kg` became 1.5. See `E-CPP-0013`.
- **A crash when a setting is missing.** You dereferenced an empty optional. See
  `E-MEM-0003`.

## Ship It

The parser joins `rc::core`, and more importantly the style does. Every function
in this curriculum that can fail in normal operation returns the failure, and the
device layer in phase 08 is built entirely this way.
