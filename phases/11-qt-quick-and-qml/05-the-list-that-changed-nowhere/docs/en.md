# The List That Changed Nowhere: What JavaScript Does to a List Property, and the Model That Says So

> A list property gives QML a copy, or a reference, depending on its type, and neither tells a view. A model does, if it says so around every change.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-02, 09-05

## The Problem

A robot interface shows a list of recent readings. The list is a property of a C++ object, the QML side pushes a new reading onto it, and the screen does not change. Or it is a model, whose row count is right and whose view is not.

## The Concept

### A push onto a list property

On Qt 6.2, one `push` from JavaScript onto each of two properties of the same object:

| property | size after | changed signals |
|---|---|---|
| `QStringList` | **3** | 1 |
| `QVariantList` | **2** | **0** |

The first is a sequence type, which QML treats as a reference and writes back. The second becomes a plain JavaScript array, and the push went to a copy. The two look the same in QML and behave differently. See `E-QT-0072`.

### A model says what happened

`SampleModel` is a list model of numbers. Its `append` is `beginInsertRows`, the change, `endInsertRows`. Two appends from QML were **2** rows and **2** `rowsInserted` signals.

A model that adds to its vector and does not make those calls is the quiet failure. After a second row the silent model had **2** rows, and the `Repeater` showing it still had **1** item; the good model had 2 rows and its `Repeater` 2 items. Every function on the silent model gave the right answer. See `E-QT-0073`.

### One signal for each row

Three appends were three `rowsInserted`, for rows 0 to 0, 1 to 1 and 2 to 2, and the `Repeater` had **3** items; a delegate read the second as **20.5** through its role name `value`. A delegate gets only the names that `roleNames` gives, and `position` is the other. `data` answers an empty `QVariant` for a row past the end (index 2 of 2), for 99, for an invalid index and for a role the model does not know. See `E-QT-0074`.

### Starting again

`clear` is `beginResetModel`, the change, `endResetModel`: **1** reset, **0** rows, and the `Repeater` with **0** items, which then followed the next append.

## Build It

`data`, `roleNames`, `append` and `clear` in `exercise/solution.hpp`.

```
rcpp verify 11-05
```

## Use It

`rc/qt/sample_model.hpp` joins its module.

```cpp
rc::qt::SampleModel model;
engine.rootContext()->setContextProperty("readings", &model);
// in QML:  ListView { model: readings; delegate: Text { text: value } }
model.append(21.5);                  // from C++ or from QML: the view follows
```

**What this does not do.** It holds numbers and appends or clears: a model that removes rows in the middle needs `beginRemoveRows` with its own range, and one that changes a row needs `dataChanged` with the roles that changed. It keeps every row, so a chart that runs for hours wants a bound (lesson 10-02 has one). And whether a list property writes back is Qt 6.2 as measured: a model is the same on every version, which is why it is the thing to use.

## What Breaks First

- **A push that went to a copy.** See `E-QT-0072`.
- **Rows changed without the begin and end.** See `E-QT-0073`.
- **A delegate with no role name, and a row past the end.** See `E-QT-0074`.

## Ship It

`rc/qt/sample_model.hpp` holds `SampleModel`.

The harness of the last lesson of this phase looks at what a view shows and not at what a model says it holds.
