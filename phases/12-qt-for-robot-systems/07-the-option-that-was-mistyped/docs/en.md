# The Option That Was Mistyped: A Command Line That Says What Is Wrong, and Does Not Exit From Inside a Test

> The parser of Qt checks the shape of a command line, not its values. Check the values, name what is wrong, and use parse() so that a test is not ended.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-05, 09-10

## The Problem

A station is started from a shortcut or a service file, with a line typed once and read for months. A letter O for a zero in the port, a limit of 99, a host with a typo in the option: each one has to be an error that says which, and not a station that starts with something else.

## The Concept

### The shape and the value

`QCommandLineParser` accepted `--port abc`, and as a number it is **0**. Given `--port 1 --port 2` it took the last, **2**, and kept both values. Its errors are about shape: `Unknown option 'hots'.`, `Missing value after '--host'.`, `Unexpected value after '--verbose'.` Everything else is text. The station says: `--port must be a whole number from 1 to 65535, not abc`, and for a repeated option, that it was given more than once. See `E-QT-0111`.

### Eleven bad lines

Each was refused, **11** of 11, with the name of what was wrong in the message:

| the line | the message names |
|---|---|
| `--port 50O1`, `0`, `65536` | `--port` |
| `--limit 99`, `-1`, `fast` | `--limit` |
| `--host ""` | `--host` |
| `--hots 10.0.0.5` | `hots` |
| `--host` at the end | `--host` |
| `10.0.0.5` on its own | the argument |
| `--verbose=yes` | `verbose` |

The numbers are read with the strict conversions of lesson 09-10. A good line was read into typed values in the long form, the short form and with an equals sign: `--host 10.0.0.5 -p 5001 --limit=2.5 -c /etc/station.ini --verbose` was 10.0.0.5, 5001, 2.5, the path, and true, and the defaults filled in everything else. See `E-QT-0112`.

### Help and version

`--help` and `--version` are good lines that set a flag, and nothing else on the line is checked when help is asked for. The usage was **13** lines and named every option. The help text of Qt does not print the default of an option, so each description says it: `default 5000`. The text wraps long lines, so a test that looks for a phrase has to put the whitespace in one piece first.

### Not ending the program

`process()` prints an error and ends the program, which in a test is the end of the test run. `parse_station_args` returns. A null for the result or for the message is allowed, and a refused line leaves the result as it was. See `E-QT-0113`.

## Build It

`parse_station_args` in `exercise/solution.hpp`. The options and `station_usage` are supplied.

```
rcpp verify 12-07
```

## Use It

`rc/qt/station_args.hpp` joins its module.

```cpp
rc::qt::StationArgs args;
QString error;
if (!rc::qt::parse_station_args(QCoreApplication::arguments(), &args, &error)) {
  std::cerr << error.toStdString() << "\n" << rc::qt::station_usage().toStdString();
  return 2;
}
if (args.help) { std::cout << rc::qt::station_usage().toStdString(); return 0; }
```

**What this does not do.** It reads one set of options. A station with sub-commands (`station record`, `station replay`) is several parsers, one for each command, and the word that chooses is a positional argument. It reads what is on the line and not the settings file: the order in which the line, the file and the defaults override each other is a decision of the program, and the usual one is that the line wins over the file and the file over the defaults. Short options are single letters and cannot be joined, so `-vp 5001` is not a thing here. And the messages are in English: the text of Qt is translated if a translation is loaded, and a message of the program itself is not.

## What Breaks First

- **A number accepted because it is text.** See `E-QT-0111`.
- **An error that does not say what.** See `E-QT-0112`.
- **A parser that ends the program.** See `E-QT-0113`.

## Ship It

`rc/qt/station_args.hpp` holds `StationArgs`, `station_usage` and `parse_station_args`.

The capstone of this phase starts from this line.
