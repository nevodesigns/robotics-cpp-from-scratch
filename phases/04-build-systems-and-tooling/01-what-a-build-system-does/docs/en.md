# What a Build System Actually Does

> You have typed cmake and ninja two hundred times. Underneath, they are a graph, a sort, and a comparison of timestamps.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-02

## The Problem

You can write C++ now. You cannot yet add a file to a project.

That sounds like a small gap and it is not. It means you cannot start a project
of your own, cannot split a growing file in two, cannot pull in a library, and
cannot explain why a build did nothing when you were certain it should have done
something. Every one of those is a build system question, and so far the build
system has been a thing that happens to you.

There is not much to it. A build system is a graph of things that depend on other
things, an order to visit them in, and a rule for deciding what is out of date.
This lesson builds that, and then reads the real `CMakeLists.txt` files in this
repository, which stop looking like incantations.

## The Concept

### Everything is a graph

A program is not one thing. It is object files that come from source files, a
library that comes from object files, an executable that comes from the library
and more objects. Each of those is a **target**, and each target depends on
others.

```
lesson_test  ->  rc_core, test_main.o
test_main.o  ->  test_main.cpp, solution.hpp
rc_core      ->  compat.hpp, pid.hpp
```

Two questions follow, and a build system exists to answer them.

### Question one: in what order?

You cannot link the executable before compiling the objects. The order must
visit every target after everything it depends on, which is a **topological
order** of the graph.

The algorithm is simple enough to write in twenty lines. Repeatedly take any
target whose dependencies have all been visited, and visit it. If at some point
nothing qualifies but targets remain, the graph has a **cycle**, and no valid
order exists.

A cycle is a real error rather than a curiosity. `A` needs `B` and `B` needs `A`
is a design mistake, and a build system that noticed it late, or looped forever,
would be worse than one that refuses immediately with the names involved.

### Question two: what actually needs doing?

Rebuilding everything is always correct and usually unbearable. The whole value
of a build system is deciding what can be skipped.

The rule is a comparison of timestamps. A target is **stale** when:

- its output does not exist yet, or
- any of its own input files is newer than its output, or
- any target it depends on is itself stale.

That third clause is the one people forget, and it is what makes staleness
spread. Touch one header and every object that includes it is stale, so every
library containing those objects is stale, so the executable is stale. One edit,
a wave through the graph.

This is also why a build can appear to do nothing. If your edit did not change
any file the graph knows about, nothing is stale, and the correct behaviour is to
do nothing at all. `ninja: no work to do` is not a failure. It is the system
telling you it disagrees with you about what changed.

### Configure and build are separate steps

CMake has a step people frequently miss, and it explains a whole family of
confusing behaviour.

**Configuring** reads your `CMakeLists.txt`, finds the compiler, finds Qt, works
out the entire graph, and writes it down in the build directory.

**Building** executes that written down graph.

So the graph is decided at configure time. Adding a source file, adding a
`Q_OBJECT` macro that needs a code generator, or installing a library that was
missing changes what the graph should be, and none of them take effect until you
configure again. That is the whole content of `E-CMAKE-0001`, and this repository
hit it during authoring: a lesson that gained a macro kept failing to link until
the build was reconfigured.

Editing the contents of a file already in the graph is different. That is picked
up automatically, because the timestamp changed and the graph already knows
about the file.

## Build It

Implement a small build system in `exercise/solution.hpp`.

- `BuildGraph::add(Target)` registers a target with its dependencies and inputs.
- `order()` returns a topological order, or a `GraphError`: `UnknownDependency`
  when a target depends on something that was never added, and `Cycle` when no
  valid order exists.
- `stale(times)` returns the targets needing work, in build order, given a table
  of file modification times. Apply all three clauses, including the third.

The result type is `rc::expected`, the one from lesson 03-02.

```
rcpp verify 04-01
```

## Use It

Now open the real thing. `cmake/RcLesson.cmake` in this repository is the
function every lesson calls, and having written the above you can read it:
`add_executable` declares a target, `target_link_libraries` adds an edge,
`target_include_directories` adds inputs, and `add_test` registers something for
`ctest` to run afterwards.

`file(GLOB ... CONFIGURE_DEPENDS)` is worth noticing, because it is the one place
this repository asks CMake to reconsider the graph when the set of files on disk
changes. Without `CONFIGURE_DEPENDS`, adding a new lesson would do nothing until
you configured again, which for a curriculum that gains lessons regularly would
be a permanent annoyance.

Ninja and Make consume the graph CMake writes. Ninja is faster mostly because its
file format is designed to be read by a machine rather than by a person.

## What Breaks First

- **The build does nothing and you are sure it should.** Nothing the graph knows
  about changed. See `E-BUILD-0002`.
- **A change to the build itself has no effect.** The graph is decided at
  configure time, so reconfigure. See `E-CMAKE-0001`.
- **You called value() on a result holding an error.** Ask which side it holds
  first. See `E-CPP-0012`.

## Ship It

`BuildGraph` joins `rc::core`. The topological sort is worth having on its own:
it is the same algorithm behind task scheduling, dependency resolution between
robot subsystems, and the startup order of the nodes in phase 17.
