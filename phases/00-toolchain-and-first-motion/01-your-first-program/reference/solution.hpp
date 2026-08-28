// The worked implementation.
//
// Read this after your own version passes, not before. Comparing two solutions
// that both work teaches you more than reading one before you have struggled.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

inline int battery_percent(int millivolts) {
  const int empty_mv = 3000;
  const int full_mv = 4200;

  // Handle the ends first. Doing this before any arithmetic means the rest of
  // the function never has to worry about producing a number outside 0 to 100.
  if (millivolts <= empty_mv) return 0;
  if (millivolts >= full_mv) return 100;

  // How far above empty are we, and how wide is the whole range?
  const int above_empty = millivolts - empty_mv;
  const int full_range = full_mv - empty_mv;

  // Multiply before dividing. Whole number division throws away the fraction,
  // so (above_empty / full_range) would be 0 for every reading below full.
  // Lesson 00-03 is entirely about this trap.
  return (above_empty * 100) / full_range;
}

#endif  // LESSON_SOLUTION_HPP
