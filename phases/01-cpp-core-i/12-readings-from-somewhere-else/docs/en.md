# Readings From Somewhere Else: Parsing a Log You Did Not Write

> A log with three bad lines in it. One was caught by checking the fields. The
> other two parsed perfectly and were caught only by counting columns.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-05, 01-08

## The Problem

Everything you have written so far computed its own numbers.

A real robot project starts the other way round. Somebody hands you a file off
the bench and asks what it says. That file was written by a different program,
probably on a different machine, possibly in a different country, and it has
blank lines in it, a comment somebody added by hand, Windows line endings, one
row that was cut short when the logger was killed, and a reading that came out
as `nan`.

The work is not the reading. It is deciding what to do with each line, and
**saying so**, rather than quietly producing a number.

## The Concept

### std::stod answers a different question

The obvious tool is `std::stod`, and it does something other than what you
want. Measured, on seven fields:

| field | stod gives | characters used | `parse_number` |
|---|---|---|---|
| `1.5` | 1.5 | 3 of 3 | accepts |
| `1,5` | **1** | 1 of 3 | refuses |
| `1.5abc` | 1.5 | 3 of 6 | refuses |
| `1.5.7` | 1.5 | 3 of 5 | refuses |
| `nan` | nan | 3 of 3 | refuses |
| `inf` | inf | 3 of 3 | refuses |
| `1e3` | 1000 | 3 of 3 | accepts |

`stod` accepted **all seven**. It throws only when the field does not begin
with a number at all, so `abc` and an empty field are caught and none of the
rows above are.

> `stod` answers whether a field **starts with** a number, not whether it
> **is** one.

Read the second row again, because it is the one that costs money. A log
written on a machine where the decimal separator is a comma reads as whole
numbers: `1,5` becomes 1 and `2,75` becomes 2, consistently, across the whole
file. Every average computed from it is wrong by a believable amount and
nothing anywhere complains.

### The one line that fixes it

`std::strtod` takes a pointer it sets to just past whatever it consumed:

```cpp
if (end != begin + text.size()) return {};   // it only began with a number
```

That is the whole difference. Three more conditions round it out: trim first,
so the carriage return a Windows file leaves on every line is gone before
anything looks at the field; check `errno` for `ERANGE`, which is how `strtod`
reports a value too big or too small and which almost nobody checks; and
reject `nan` and `inf`, because a sensor cannot read either and one of them
poisons every average computed from the file.

One accepted case is worth knowing before you meet it: `strtod` reads
hexadecimal, so a field of `0x10` is the number **16**. That is a correct
answer to a question you probably did not mean to ask.

### What checking the fields cannot catch

Here is the part that writing this lesson taught me, having got it wrong first.

I expected a row cut short and a decimal comma row to fail parsing. Neither
does.

```
0.2,1.60            a row cut short when the logger was killed
0.4,1,70,2.45       a decimal comma, in a comma separated file
```

The first is a perfectly good **two** column row. The second splits into
**four** fields and every one of them is a valid number. Nothing about either
is malformed at the field level.

Measured on a log with three bad lines in it:

| | caught by |
|---|---|
| `nan` in a field | parsing the field |
| a row cut short | **counting columns** |
| a decimal comma | **counting columns** |

One of three. A reader that validates fields and not shape reports a clean load
and hands back rows of mixed width, and whatever indexes column 2 afterwards
gets a different quantity depending on which row it is on.

So the width has to be enforced, and `read_log` takes it from the first usable
row rather than from an argument, so a caller does not have to know in advance
and a file that is consistently four columns is read as four columns.

### Count the two kinds separately

```
lines read          9
blank or comment    3
field not a number  1
wrong width         2
width taken as      3
kept                3
first bad line      6
```

Unparsable fields suggest a sensor fault. Wrong width rows suggest a format
mismatch, and frequently a locale one: a file whose rows are consistently one
column too wide was written with a decimal comma, and the fix is to read it
with a semicolon separator rather than to patch the rows. One number for both
leaves the reader guessing which.

`first_bad_line` is one based and counts **every** line including the blanks
and comments, because that is what an editor shows and the whole point of the
number is that somebody can go and look.

And never return only the rows that worked. A reader that silently filters
cannot be distinguished from one that found nothing wrong, and the difference
between two bad lines and two thousand is the difference between a glitch and
a format mismatch.

### Take a stream, not a filename

```cpp
LogReport read_log(std::istream& input, char separator);
```

A test hands it a `std::istringstream` holding exactly the awkward content it
wants, in one line, with no file and no cleanup. A program hands it a
`std::ifstream`.

This is the same idea as the replay sensor in 05-02 and the injectable clock in
03-05: **depend on the interface, not on the device.** It is the single most
useful habit in this curriculum for making things testable, and here it is the
difference between writing twenty malformed cases and writing two.

The suite does both: five tests on strings, and one that writes a real file,
reads it back and deletes it, to prove the other end is connected.

## Build It

Implement `parse_number`, `parse_row` and `read_log` in
`exercise/solution.hpp`.

```
rcpp verify 01-12
```

The suite measures what `std::stod` does with seven fields and compares,
checks the edges of a single field, shows what a row parser can and cannot
see, reads a log with three different problems in it, and then does the same
through a real file on disk.

## Use It

**Never use `std::stod` or `std::atof` on input you did not write.** Require the
whole field.

**Trim before you look.** The carriage return is there more often than you
expect.

**Enforce the width.** It is the only thing that catches a truncated row or a
locale mismatch.

**Report counts, not just rows.** And report the first bad line number.

**Take a stream.** Open the file at the caller, which is the only place that
knows whether a missing one is fatal.

**Reject `nan` at the boundary**, where it is one line, rather than hunting it
in an average later.

## What Breaks First

- **A field that parsed and was not a number.** See `E-IO-0014`.
- **A row that parsed and was the wrong shape.** See `E-IO-0015`.
- **A reader that takes a filename.** See `E-IO-0016`.

## Ship It

`parse_number`, `parse_row` and `read_log` join `rc::core`. This is the first
lesson in the curriculum where the data came from outside the program, and
every later phase that reads a recorded trace, a calibration file or a
configuration uses it rather than `stod`.
