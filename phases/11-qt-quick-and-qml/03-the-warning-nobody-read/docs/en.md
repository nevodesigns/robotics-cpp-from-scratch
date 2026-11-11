# The Warning Nobody Read: Capturing What Qt Prints So That a Test Can Fail On It

> Qt reports by printing. A scope that keeps what is printed turns a warning in a log into a thing a test can count and fail on.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-01, 09-03

## The Problem

Lesson 11-01 found that most mistakes in QML do not stop a load: they are warnings. It hooked the `warnings` signal of the engine. But Qt prints much more than the engine reports, and everything goes the same way, to standard error, where a test run does not look. A suite can pass with forty warnings in its log.

## The Concept

### A handler that throws

A QML file with `Component.onCompleted: nothingCalledThis.value` loads, and creates its object. The scope kept **1** warning, of type warning, with its file and line:

`.../throws.qml:2: ReferenceError: nothingCalledThis is not defined`

Without the scope it went to standard error and the test passed. See `E-QT-0066`.

### What arrives

`console.log` in QML was a debug message, `console.warn` a warning, and `qWarning` from C++ a warning that carried the file and line of the C++ that printed it. `qInfo` was an info and `qDebug` a debug. All five messages were kept, in order.

### Putting the handler back

A message handler is global, and a scope that does not put it back leaves a function installed that writes into a destroyed object. The scope remembers the old handler and restores it. A nested scope hands back to the one around it: the outer scope kept **2** messages (the ones printed outside the inner scope) and the inner kept its own **2**. After the scopes ended the old handler was back and had seen **2** (the one before and the one after). With `forward`, each message was kept and passed on. See `E-QT-0067`.

### Other threads

Qt calls the handler from whichever thread printed. **8** threads printing **1000** messages each: **8000** kept, because the list is guarded by a mutex. See `E-QT-0068`.

## Build It

`MessageCapture::handle` in `exercise/solution.hpp`. The constructor and destructor are supplied.

```
rcpp verify 11-03
```

## Use It

`rc/qt/message_capture.hpp` joins its module.

```cpp
rc::qt::MessageCapture capture;
run_the_interface();
RC_CHECK_EQ(capture.count(QtWarningMsg), 0u);        // a clean run says nothing
RC_CHECK(!capture.contains("TypeError"));
```

**What this does not do.** It keeps text, and not meaning: a warning that you expect is counted with the ones you do not, so a test that provokes one has to say how many to expect. It is a process-wide handler, so two scopes on two threads at once see the messages of each other. It does not change what Qt prints or how loud it is: a category that is switched off by a logging rule is not printed and is not kept. And it does not replace the `warnings` signal of the engine, which carries the file and line of a QML error as data.

## What Breaks First

- **A warning that nobody read.** See `E-QT-0066`.
- **A handler left installed.** See `E-QT-0067`.
- **A list written from two threads.** See `E-QT-0068`.

## Ship It

`rc/qt/message_capture.hpp` holds `CapturedMessage` and `MessageCapture`.

The last lesson of this phase puts it in the test harness that a QML interface is loaded through.
