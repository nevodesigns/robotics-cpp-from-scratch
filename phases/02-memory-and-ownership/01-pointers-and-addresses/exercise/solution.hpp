#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// One temperature reading from a sensor, with the moment it was taken.
struct Reading {
  double celsius = 0.0;
  int milliseconds = 0;
};

// The address of the first reading above limit, or nullptr when there is none.
//
// Must also answer nullptr when readings is null or count is zero or negative,
// rather than reading from memory it was never given.
inline const Reading* find_first_above(const Reading* readings, int count, double limit) {
  // TODO
  (void)readings;
  (void)count;
  (void)limit;
  return nullptr;
}

// Exchanges two readings through their addresses.
// Does nothing when either pointer is null.
inline void swap_readings(Reading* a, Reading* b) {
  // TODO: remember that assigning to a moves the pointer, while assigning to
  // *a changes the reading it points at. Only one of those is what you want.
  (void)a;
  (void)b;
}

// The address of the largest reading, or nullptr when there is none.
inline const Reading* highest(const Reading* readings, int count) {
  // TODO
  (void)readings;
  (void)count;
  return nullptr;
}

#endif  // LESSON_SOLUTION_HPP
