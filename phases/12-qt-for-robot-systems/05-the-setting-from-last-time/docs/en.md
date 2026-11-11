# The Setting From Last Time: What a Station Keeps Between Runs, What It Reads Back, and Files Written by an Older Version

> A settings file is written by the program and read by people, other machines and older versions. Read it strictly, write it when asked, and migrate it once.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-10, 12-03

## The Problem

A station remembers where the robot is and how fast it may be driven. Between one run and the next, the file is copied to another computer, edited with a text editor, truncated by a crash and left behind by a release that is two versions old. The station has to start from it every time.

## The Concept

### A file is text

A file written by hand and read fresh by `QSettings`: the port `abc` as an integer was **0** with `ok` false, and a plain `toInt()` does not say so; the flag `no` as a bool was **true** (lesson 09-10); a stored number had the type **QString**. A speed limit of 0 is a robot that does not move, and nothing said why. The station read the same file with the strict conversions of 09-10: the port was the default **5000**, and one note said `robot/port: not a whole number, using 5000`. See `E-QT-0105`.

### Missing, wrong and out of range

A value that is not in the file is the default and not a problem: an empty file had no notes. Values that are there and wrong each give a note:

| in the file | the station used |
|---|---|
| `limit_mps=99` | **5** (the nearest end of 0 to 5) |
| `limit_mps=-3` | **0** |
| `limit_mps=nan` | the default 1.0, with a note |
| `robot/port=70000` | the default **5000** |
| `display/invert_y=maybe` | false |

The file with the first three of the 99, the 70000 and the maybe gave **3** notes.

### Saved when asked

`set_*` puts a setting in memory. Before `save()` the file **did not exist**; after it, the file held host, port and limit, and the version. A station that is killed before it saves has kept nothing, and a second `QSettings` object in the same process sees an unsaved value, which makes the test of a save a test of the file and not of the object. A port outside 1 to 65535 is refused by `set_port`, and the good port is still there. See `E-QT-0106`.

### An older file

Version 1 kept `robot_ip` and `speed_limit_mm`, in whole millimetres a second. Version 2 keeps `robot/host` and `limit_mps`, in metres. A file of the first version: host **192.168.1.20**, limit **1.5** m/s (from 1500), version **2**. After saving the old names were **gone** and the new ones were in. Starting again changed nothing and the limit was still 1.5, not 0.0015: it is migrated once. A speed that was not a number was dropped, with a note. See `E-QT-0107`.

## Build It

`port`, `limit_mps`, `save` and `migrate` in `exercise/solution.hpp`. `host`, `invert_y`, the setters and `strict_int` are supplied.

```
rcpp verify 12-05
```

## Use It

`rc/qt/station_settings.hpp` joins its module.

```cpp
rc::qt::StationSettings settings(QDir::home().filePath(".config/station.ini"));
link.connect_to(settings.host(), settings.port());
for (const QString& note : settings.problems()) log_warning(note);
// after the operator changes something:  settings.save();
```

**What this does not do.** It reads and writes a file, and does not make the file safe: a person with a text editor can put anything in it, and the notes are what tell them. It migrates from one older version, and a station with ten versions of its file needs a step for each, in order, each one testing for the version it brings forward. `QSettings` writes the file in one piece when it saves, and two stations that save the same file at the same time are two writers. It does not hold secrets: a password in an INI file is a password in a text file. And the file is the same on every platform here, which is why it is an INI file and not the registry.

## What Breaks First

- **A value read as zero.** See `E-QT-0105`.
- **A setting that was never saved.** See `E-QT-0106`.
- **An older file read as defaults.** See `E-QT-0107`.

## Ship It

`rc/qt/station_settings.hpp` holds `StationSettings`.

The capstone of this phase starts from it and saves to it.
