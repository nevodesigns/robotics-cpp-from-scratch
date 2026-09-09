# The Error You Only Get When It Runs: What QML Checks, and When

> Twelve mistakes in a QML file. Six are refused before anything is created,
> three load and whisper, and three load and say nothing whatever.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 09-01, 05-04

## The Problem

You have spent two phases on Qt in C++, and every mistake you made was caught by
the compiler before the program existed. Misspell a member and it will not
build. Pass the wrong type and it will not build. That is the deal C++ offers,
and it is why the phase 10 plotter worked the first time it ran.

QML is a different deal, and this lesson is about what the new deal actually is
rather than about how to write a button.

QML is loaded, not compiled. Your C++ builds cleanly whatever nonsense is in the
`.qml` file next to it, because at build time it is a text file. What happens
afterwards splits three ways, and only one of the three is loud.

## The Concept

### Tier one: what QML can see by reading

Some mistakes are visible in the document. QML refuses those, and it refuses
them before creating anything.

| case | loaded | errors | first location |
|---|---|---|---|
| a syntax error | NO | 1 | 2:15 |
| a type that does not exist | NO | 1 | 2:1 |
| a property that does not exist | NO | 1 | 2:8 |
| a missing import | NO | 1 | 1:1 |
| a string where a number goes | NO | 1 | 2:15 |
| a colour that is not one | NO | 1 | 2:20 |

Six for six, each with a line and a column. This tier is better than its
reputation: `Item { width: "wide" }` is refused, and so is
`Rectangle { color: "notacolour" }`. QML is genuinely strict about what is
written down.

The catch is not that this tier is weak. It is that the API most people use
throws the detail away. `QQmlApplicationEngine::load` gives you back an empty
`rootObjects()` and nothing else, so a caller that checks
`rootObjects().isEmpty()` has reduced a url, a description, a line and a column
to one bit. `QQmlComponent`, the primitive underneath, keeps the list.

### Tier two: what only running the binding can tell you

Now the ones QML cannot see by reading.

```qml
Item { width: robto.speed }
```

Is `robto` a name that resolves? The document cannot say. It depends on what the
engine's context happens to contain when the binding runs. So QML loads the
file, creates the object, evaluates the binding, gets undefined, and leaves
`width` at its default.

| case | loaded | warnings | width |
|---|---|---|---|
| a name that does not exist | yes | 1 | 0 |
| a typo on a context property | yes | 1 | 0 |
| a binding loop | yes | 1 | 2 |

Everything loaded. The interface exists and draws. The panel is just the wrong
size.

There is a channel for this, and it is the single most useful thing in the
lesson: `QQmlEngine` has a signal called `warnings`, carrying a
`QList<QQmlError>` with a line and a column. **Nothing is connected to it unless
you connect it.** By default those warnings go to stderr, and in a running
application nobody is watching stderr.

Here is what the ordinary API says about the file with the undefined name:

```
component.status()   == QQmlComponent::Ready
component.errors()   is empty
root->property("width") == 0
```

Every question you can think to ask says the file is fine.

### Tier three: what nothing reports

And then there is the tier with no channel at all.

| case | loaded | errors | warnings | width |
|---|---|---|---|---|
| zero divided by zero | yes | 0 | 0 | 0 |
| undefined assigned to a number | yes | 0 | 0 | 0 |
| anchors against a parent it has not got | yes | 0 | 0 | 0 |

`0/0` is `NaN` in JavaScript, and `NaN` assigned to a `real` becomes zero.
`undefined` becomes zero. `anchors.fill: parent` with a null parent does
nothing. None of these is an error by any definition QML has, so no amount of
hooking will surface them.

### The rule all twelve obey

> QML checks what it can read, and runs the rest.

`width: "wide"` is a string in the document, and it is refused. `width: 0/0` is
an expression, and it is run. That one sentence predicts every row in all three
tables, and it is worth more than a list of gotchas because it tells you which
tier a new mistake will land in before you try it.

It also tells you what to do about each. Tier one needs the error list kept
rather than discarded. Tier two needs a hook nothing installs for you. Tier
three needs a test, because there is nothing else.

## Build It

Implement `problem_from`, the `QmlHost` constructor and `load` in
`exercise/solution.hpp`.

```
rcpp verify 11-01
```

The suite writes twelve small QML files, loads each one, and reports which tier
it landed in. It also loads a file through a bare `QQmlComponent` with nothing
connected, to show what an application that has not thought about this sees.

## Use It

**Connect `QQmlEngine::warnings` in every application, on the first line.** It
is one connection and it converts an invisible failure into a countable one.

**Fail the build when the count is not zero.** A warning during load is a defect,
not a diagnostic to read later. `warning_count()` exists so a test can say so.

**Return the errors, not a bool.** `rc::expected<QObject*,
std::vector<QmlProblem>>` costs nothing and lets a caller print the file, the
line and the column QML already worked out.

**Assert the values, not the load.** The load succeeding is nearly no
information. That the width is 100 because the file says 100 is information.

**Keep the component alive as long as its object.** Destroying a `QQmlComponent`
while its object is still in use takes the object's type information with it.

## What Breaks First

- **A binding that quietly evaluated to nothing.** See `E-QT-0018`.
- **A mistake nothing reports at all.** See `E-QT-0019`.
- **A failed load reported as a boolean.** See `E-QT-0020`.

## Ship It

`QmlProblem` and `QmlHost` join `rc::qt` beside the path view and the worker.
Every QML lesson after this one loads its files through the host, so a binding
that fails silently in phase 11 or 12 fails a test instead.
