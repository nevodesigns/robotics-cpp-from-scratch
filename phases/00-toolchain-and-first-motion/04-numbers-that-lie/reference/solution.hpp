#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

inline double adc_to_volts(int raw) {
  // One fractional operand makes the whole expression fractional, so the
  // division keeps its remainder. Writing 4095.0 rather than 4095 is the entire
  // fix, and it is easy to miss in review, which is why it is worth a comment.
  return (raw * 3.3) / 4095.0;
}

inline int percent_of(int part, int whole) {
  // Guard the impossible input before doing arithmetic on it. Dividing by zero
  // with whole numbers is undefined behaviour, which means the program is
  // allowed to do anything at all, including appearing to work.
  if (whole == 0) return 0;

  // Multiply before dividing so the ratio survives, then add half a divisor to
  // round to nearest instead of always rounding towards zero.
  return (part * 100 + whole / 2) / whole;
}

inline double average(const int* samples, int count) {
  if (count <= 0) return 0.0;

  // The running total is a fractional number, so the final division is too.
  // Summing into a double also avoids overflowing an int on a long run of
  // large samples.
  double sum = 0.0;
  for (int i = 0; i < count; ++i) {
    sum += samples[i];
  }
  return sum / count;
}

#endif  // LESSON_SOLUTION_HPP
