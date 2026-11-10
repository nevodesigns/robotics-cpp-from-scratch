# Where Did This Value Come From: Settings from Four Places, Which One Wins, and the Typo That Was Silently Ignored

> A robot's speed limit can be set by a default, a file, an environment variable and a command line argument. When it
> is wrong, the first question is which of the four is in force, and the second is why a typo changed nothing.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-13, 03-02, 03-03

## The Problem

A program is told things in four ways, and each is somebody's convenience. A default in the code. A configuration file beside the program. An environment variable that a launch script exports. An argument on the command line. All four can name the same setting, and the program has to pick one.

Lesson 01-13 read the command line and lesson 03-02 read a file. What neither did was put them together, and the places where they meet are where robots end up running at the wrong speed with nobody able to say why.

## The Concept

### Which one wins

Weakest first: default, file, environment, command line. A person who types something on the command line means it more than a file written last month does.

`Settings::set(source, key, value)` stores a value with its source, and refuses to replace a stronger one. This matters because the order of reading is an accident of how `main` was written. The command line said 9000, then the file 8080, then the default 80, and the port was **9000** from the command line. The same source set twice
behaved as a file does: the later line (10) beat the earlier (5). See `E-BUILD-0014`.

### Where it came from

Each value remembers its source, and `explain` writes it:

```
log_level = debug (environment)
timeout = 30 (default)
nothing is not set
```

A start-up log that prints one line like that for every setting settles most arguments. See `E-BUILD-0016`.

### The typo that did nothing

A map of strings accepts any key, and the program reads only the ones it knows. `speed_limt=0.5` sits in the map and the robot goes at full speed. After all the sources are read, `unknown_keys` lists what was set and is not known.

To say what was probably meant, `suggest` uses **edit distance**: the number of single-character changes (a letter changed, left out or added) that turn one word into another. Of five unknown keys:

| typed | suggestion |
|---|---|
| `speed_limt` | speed_limit |
| `wheelbase` | wheel_base |
| `prot` | port |
| `LOG_LEVEL` | none (the wrong case is eight edits) |
| `colour_scheme` | none |

A suggestion has to be within two edits and fewer than the length of the word, so that a one or two letter word is not matched to every name. A swap of two letters, `prot` for `port`, is **two** edits, not one. `kitten` to `sitting` is 3. See `E-BUILD-0015`.

## Build It

`edit_distance`, `Settings::set`, `get`, `unknown_keys` and `suggest` in `exercise/solution.hpp`. The source ranks, `explain` and `source_name` are given.

```
rcpp verify 04-12
```

## Use It

`rc/core/layered_config.hpp` joins `rc::core`.

```cpp
Settings s;
s.set(kDefault, "speed_limit", "1.0");
// ... read the file, then the environment, then the arguments ...
for (const std::string& key : s.unknown_keys(known))
  std::cerr << "unknown setting " << key << ", did you mean " << suggest(key, known) << "?\n";
std::cerr << s.explain("speed_limit") << "\n";
```

**What this does not do.** Values are strings: turning them into numbers, and refusing the ones that are not, is lesson 03-15. It has four fixed sources and no way to add another, such as a launch file. It does not know that two names mean the same setting, or that some settings must be given together. A suggestion is a guess:
it is shown to a person and never acted on. And it cannot tell a setting meant for another program, which a shared environment will contain.

## What Breaks First

- **A weaker source read last.** It wins. See `E-BUILD-0014`.
- **A key nobody reads.** A silent typo. See `E-BUILD-0015`.
- **A value with no source.** Nobody can say where it came from. See `E-BUILD-0016`.

## Ship It

`rc/core/layered_config.hpp` holds the source ranks, `edit_distance`, `Settings` and `suggest`.

Lesson 04-11 asked what a build's output depends on. This asks the same of a running program: which of four places set this.
