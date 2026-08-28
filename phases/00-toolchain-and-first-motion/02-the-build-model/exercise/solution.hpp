// Declarations only. No function bodies live in this file.
//
// A declaration is a promise that a function exists somewhere. The linker is
// what checks the promise was kept.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Converts a raw temperature sensor count into degrees Celsius.
// One count is 0.0625 degrees, so 320 counts is 20.0 degrees.
double celsius_from_raw(int raw);

// True when the temperature is above 80 degrees Celsius.
bool is_overheating(int raw);

#endif  // LESSON_SOLUTION_HPP
