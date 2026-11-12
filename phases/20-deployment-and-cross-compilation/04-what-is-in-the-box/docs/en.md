# What Is in the Box: A Release Bundle With a Manifest That Checks Itself, and the Four Ways Files Differ From It

> The robot should refuse to run a release it cannot show is the release it was sent.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-03, 20-01, 00-11, 08-03

## The Problem

The program is built for the robot. It now has to **get there**: over a wifi link at a field site, from a USB stick, through a download that stops half way. A bundle that arrives with one byte wrong, a file missing or a file left over from an older release is the usual cause of a robot that behaves differently from the one that was tested, and the behaviour is rarely a clean crash. The remedy is the oldest one: a **list of what should be there**, and a check, on the robot, before anything is run.

## The Concept

### A manifest that is the same every time, and checks itself

`make_manifest` writes a text file:

```
rcbundle 1
version 1.4.2
file bin/robot_node 1204 4d6a7c40c3373a6d
file etc/robot.conf 12 5ccba7e89fa93bc9
file lib/libmsgs.so 4 a7f809a28b837383
end 5764788296d98938
```

One line for each file with its path, its size and a 64 bit hash of its contents, **sorted by path whatever order the files came in**: the same release given in two orders gave the same text, so the manifest can itself be compared and hashed. The last line is the hash of **everything above it**. The hash is FNV-1a, whose published values the test checks (the empty text is `cbf29ce484222325`, `a` is `af63dc4c8601ec8c`), and it is **not a defence against someone who means it**: anyone can recompute it. It catches accident. A real release is signed, which is the last section below. An empty release is a valid manifest with no files. See `E-DEPLOY-0010`.

### Refusing what is not a manifest

`parse_manifest` is strict, and tells you **why** in words:

- cut off before its end line: `no end line`. A download that stopped is the most common damage there is, and this is the line that finds it.
- one digit of a size changed: `bad checksum`. So is a changed version. The end line is the manifest's own seal.
- and, from the first line on: `bad header`, `bad version line`, `bad file line 3` (with the line number), `unsafe path`, `duplicate path`, and text after the end line.

Ten more malformed manifests, each with its own reason. A parser that is lenient here, which skips a line it does not understand, will accept a manifest with half its files. See `E-DEPLOY-0010`.

### Paths that cannot escape

A path in a manifest is a file name on the robot, and the robot's updater will write there. Of 15 paths tried, **4** were accepted: `bin/robot_node`, `etc/robot.conf`, `a`, `lib/libmsgs.so.1.2`. Refused: `../etc/passwd`, `bin/../../x`, `/etc/passwd`, `bin//x`, `./bin/x`, `bin/`, an empty path, `C:/x`, a backslash, a space, `..`; and 256 characters. A bundle with a path of `../../etc/passwd` is the classic way an archive overwrites the system that unpacks it, and it **is the manifest's job to refuse it before any file is written**, since by the time the files are being unpacked it is too late. `make_manifest` refuses the same ones, with `unsafe path: ../x` and `duplicate path: a`. See `E-DEPLOY-0011`.

### How files differ

`verify_files` compares the files on the robot with the manifest, and a file can differ in four ways. A robot had: one byte of `bin/robot_node` changed (the same size), `etc/robot.conf` cut short by a byte, `lib/libmsgs.so` never arrived, and `bin/old_tool` left behind by release 1.3:

| path | result |
|---|---|
| bin/robot_node | hash |
| etc/robot.conf | size |
| lib/libmsgs.so | missing |
| bin/old_tool | extra |

The first is the one a size check cannot see. The last is the one that nobody checks: a **stale file** from an earlier release, which a program may load by name, is part of what is running without being part of the release. Two files with each other's contents were both reported. A path given twice among the files counts once. And the flips: **1000 of 1000** single bit flips in a 4096 byte file were caught, as they should be with a 64 bit hash, against odds of about 1 in 1.8e19 for each that is missed by chance. See `E-DEPLOY-0012`.

## Build It

`hash64`, `safe_path`, `make_manifest`, `parse_manifest` and `verify_files` in `exercise/solution.hpp`. `hex16` and the types are given.

```
rcpp verify 20-04
```

## Use It

`rc/deploy/bundle.hpp` joins `rc::deploy`.

```cpp
const auto parsed = rc::deploy::parse_manifest(read_file("MANIFEST"));
if (!parsed.ok) return fail(parsed.error);
const auto differences = rc::deploy::verify_files(parsed.manifest, files_unpacked);
if (!differences.empty()) return fail("the bundle does not match its manifest");
```

**What this does not do.** It checks that a bundle is **what the manifest says**, not that the manifest is **what you sent**. Both can be replaced by someone who can write to the place they arrive, and the hash is not secret. A real release is **signed** with a key that stays at the build machine, the robot holds the public half and refuses anything that does not verify, and the manifest is the thing signed. That needs a proper cryptographic library (an Ed25519 or RSA signature, and SHA-256 for the hashes), which this curriculum does not write, since writing one is the way to write a vulnerable one. The manifest lists files and not **permissions**, owners or symbolic links, each of which matters on a robot. It reads the whole bundle into memory. And it does not defend against a **rollback** to an older, signed, vulnerable release: that is the version rule of a later lesson.

## What Breaks First

- **A manifest that cannot tell it was cut short or edited.** See `E-DEPLOY-0010`.
- **A path that leaves the directory.** See `E-DEPLOY-0011`.
- **A file that was never in this release.** See `E-DEPLOY-0012`.

## Ship It

`rc/deploy/bundle.hpp` holds `FileData`, `Entry`, `Manifest`, `hash64`, `hex16`, `safe_path`, `MakeResult`, `make_manifest`, `ParseResult`, `parse_manifest`, `Mismatch`, `Difference` and `verify_files`.
