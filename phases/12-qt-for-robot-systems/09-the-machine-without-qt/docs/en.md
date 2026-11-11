# The Machine Without Qt: A Station That Ran Where It Was Built, What Was Not Shipped, and the Path That Only Exists Here

> A program that runs where it was built has been run with the build around it. List what must be shipped, and look for the paths that only the builder has.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-05, 04-14

## The Problem

The station works on the machine it was built on and is copied to a robot with nothing on it. It does not start, or it starts without a screen, and the loader says that a library or a plugin called platforms/xcb is nowhere. Everything it needed was found on the build machine, by a path variable or a directory the build put there, and none of it was copied.

## The Concept

### What must be in the folder

`check_deployment` takes the folder to be shipped and a list of patterns, each relative to it and with wildcards in its last part: `station*`, `*Qt6Core*`, `platforms/*offscreen*`, `qt.conf`. A complete folder passed with **0** missing and **0** stray paths, and the same patterns matched the names of a Windows build (`station.exe`, `Qt6Cored.dll`, `qoffscreen.dll`). See `E-QT-0117`.

### What is missing

With the platform plugin and the core library taken out, the report was `*Qt6Core*` and `platforms/*offscreen*`: the patterns that were asked for, in the order they were asked for, and **2** of them. A directory that is not there matches nothing and is not an error. A folder that does not exist had **4** of **4** missing, and nothing asked for is nothing missing.

### The path that only exists here

A `qt.conf` that a build machine wrote: a prefix of `/home/builder/Qt/6.2/gcc_64`, and two Windows paths, `C:\Qt\6.5.3\msvc2019_64\bin` and `C:/Qt/6.5.3/data`. All **3** were stray paths, written as `qt.conf: <path>`; a relative `Plugins=plugins`, and a log path that exists, were not. `path_tokens` finds paths as written on either system, up to a space, comma, semicolon or quote: four in one test line, none in `Plugins=plugins`. See `E-QT-0118`.

### The other half

This check says what is in the folder. Where Qt looks for plugins is another question: the program knew **at least one** library path, and a `qt.conf` next to the executable is what changes them. A deployed station that is started with the wrong plugin path starts without a screen. See `E-QT-0119`.

## Build It

`has_match` and `check_deployment` in `exercise/solution.hpp`. `path_tokens` is supplied.

```
rcpp verify 12-09
```

## Use It

`rc/qt/deploy_check.hpp` joins its module.

```cpp
const rc::qt::DeployReport report = rc::qt::check_deployment("/opt/station-1.4",
    {"station*", "*Qt6Core*", "platforms/*xcb*", "qt.conf"}, {"qt.conf", "station.ini"});
for (const QString& m : report.missing) std::cerr << "missing: " << m.toStdString() << "\n";
for (const QString& s : report.stray_paths) std::cerr << "stray: " << s.toStdString() << "\n";
```

**What this does not do.** It looks at names and at text, and not at what a library needs: a program links to libraries that link to others, and the list of what is needed comes from a tool that reads the program (`ldd` on Linux, `windeployqt` on Windows), which this does not run. It checks for the file and not that it is the right version. It looks for paths that do not exist on the machine it runs on, so it has to be run on the machine that is like the target and not on the build machine, where they all do. And a plugin that is found is not a plugin that loads: a plugin built for another version of Qt is refused by Qt when it starts.

## What Breaks First

- **A file that was not shipped.** See `E-QT-0117`.
- **A path from the build machine.** See `E-QT-0118`.
- **A check run where everything exists.** See `E-QT-0119`.

## Ship It

`rc/qt/deploy_check.hpp` holds `DeployReport`, `path_tokens`, `has_match` and `check_deployment`.

The capstone of this phase is checked with it before it is called shippable.
