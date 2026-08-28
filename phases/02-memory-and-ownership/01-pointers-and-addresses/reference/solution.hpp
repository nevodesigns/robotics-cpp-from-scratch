#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

struct Reading {
  double celsius = 0.0;
  int milliseconds = 0;
};

inline const Reading* find_first_above(const Reading* readings, int count, double limit) {
  // Guard the inputs before touching memory. A null array with a count of ten
  // is a caller mistake, and reading from it would be undefined behaviour that
  // often looks like it worked.
  if (readings == nullptr || count <= 0) return nullptr;

  for (int i = 0; i < count; ++i) {
    if (readings[i].celsius > limit) {
      // &readings[i] is the address of that element. Returning it is safe here
      // because the array belongs to the caller and outlives this call.
      return &readings[i];
    }
  }
  return nullptr;
}

inline void swap_readings(Reading* a, Reading* b) {
  if (a == nullptr || b == nullptr) return;

  // The star is doing the work. Without it this would exchange two local copies
  // of the addresses and the caller would see nothing change at all.
  const Reading held = *a;
  *a = *b;
  *b = held;
}

inline const Reading* highest(const Reading* readings, int count) {
  if (readings == nullptr || count <= 0) return nullptr;

  const Reading* best = &readings[0];
  for (int i = 1; i < count; ++i) {
    if (readings[i].celsius > best->celsius) best = &readings[i];
  }
  return best;
}

#endif  // LESSON_SOLUTION_HPP
