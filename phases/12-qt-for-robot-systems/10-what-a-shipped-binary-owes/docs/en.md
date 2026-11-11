# What a Shipped Binary Owes: The Licence of Each Qt Module You Link, How You Link It, and What You Must Hand Over

> What a program owes for using Qt depends on which modules it links and how. Find out before shipping, and say unknown when the table does not know.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-09, 04-13

## The Problem

A station is ready to ship, and someone asks what licence it is under. The answer depends on which parts of Qt it links, whether it links them dynamically, and what goes out with it. Finding that out after the program is in a customer hands is how a company gets a letter. **This lesson is not legal advice**, and its table is not a licence: it is a place to start from, for the open source licences as Qt describes them for the 6.2 series. A maker with a lawyer, or with a commercial licence from Qt, asks the lawyer, and anyone shipping reads the licence of the Qt that is shipped.

## The Concept

### The shape

Most of the parts a control station uses, Core, Gui, Widgets, Network, Qml and Quick among them, are offered under the **LGPL 3.0**. It lets a program that is not free use them if the program links dynamically, so that the person who has it can put in another build of Qt, and hands over the licence text with a notice that it uses Qt, and makes the source of the Qt it ships available. A few parts, **Charts**, **Data Visualization**, **Virtual Keyboard** and the **Wayland compositor**, are offered under the **GPL 3.0 only** or a commercial licence, and a program that links one of those and is given to others is the GPL 3.0 as a whole, with its source, or has to be built with the commercial licence.

### Names

A module is spelt `Qt6Core` in a build file, `QtCore` in an include and `Core` in CMake. `normalise` makes all of them `core`, and takes the prefix once: `Qt6Qt6Core` is `qt6core`, which is not a module. A name that is only the prefix is left alone.

### The table, and unknown

`licence_of` has what is certain: **11** modules tested were LGPL, **4** were GPL only, and `Frobnicate`, `Quick3D`, `Multimedia` and the empty name were **unknown**. A module that is not in the table is not assumed to be anything: guessing LGPL for a module that is not is how the wrong thing ships. Extending the table is a matter of reading the licence page of Qt for your version.

### The assessment

Core, Gui, Widgets and Network, linked dynamically, with the licence text and an offer of source: **0** findings. The same program linked statically was not ok. Each fault is its own finding with the thing named: a GPL only module (`gpl-only`), a static link of LGPL modules (`static-lgpl`), no licence text (`no-licence-text`), no offer of source (`no-source-offer`) and a module not in the table (`unknown-module`). Static linking of only a GPL only module is not also a static finding: there is nothing LGPL to link. See `E-BUILD-0023` and `E-BUILD-0024`.

### Everything at once

A list with Core, Qt6Charts, Multimedia, Charts, QtCharts and VirtualKeyboard, linked statically with neither text nor offer: **6** findings, in this order: gpl-only, unknown-module, gpl-only, static-lgpl, no-licence-text, no-source-offer. A module spelt three ways (Qt6Charts, Charts, QtCharts) was one finding. See `E-BUILD-0025`.

## Build It

`normalise`, `licence_of` and `assess` in `exercise/solution.hpp`. The licence and linking types are supplied.

```
rcpp verify 12-10
```

## Use It

`rc/core/qt_licence.hpp` joins its module.

```cpp
const rc::core::Assessment a = rc::core::assess({"Core", "Gui", "Widgets", "Network"}, rc::core::Linking::kDynamic, /*ships_licence_text*/ true, /*offers_source*/ true);
for (const rc::core::Finding& f : a.findings) std::cerr << f.code << ": " << f.text << "\n";
```

**What this does not do.** It is a table of a few modules, and it does not know about the version: a module that moved from one licence to another between versions of Qt is a module for which the table has to be read again. It does not read your build to find what you link: the list is given to it, and a program links more modules than it names, because one module needs another. It knows nothing of the other libraries in the program, whose licences have their own terms, of the plugins you ship, or of the fonts and images. It checks that you say you ship the text and an offer, and not that you do. And it is not legal advice: it says what to check.

## What Breaks First

- **A GPL only module linked without anyone noticing.** See `E-BUILD-0023`.
- **The LGPL modules linked statically.** See `E-BUILD-0024`.
- **A notice or an offer that was not shipped, and a module that was guessed.** See `E-BUILD-0025`.

## Ship It

`rc/core/qt_licence.hpp` holds `Licence`, `Linking`, `Finding`, `Assessment`, `normalise`, `licence_of` and `assess`.

The capstone of this phase is assessed with it before it is called shippable.
