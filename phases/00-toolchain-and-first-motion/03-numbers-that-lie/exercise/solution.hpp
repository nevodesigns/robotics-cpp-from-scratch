// Three conversions, three classic numeric bugs. Fix all three.
//
// Each function below compiles and runs. Each one is also wrong. The tests will
// tell you exactly how.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Converts a raw count from 0 to 4095 into volts from 0.0 to 3.3.
//
// BUG: raw / 4095 is whole number division, so it is 0 for every reading below
// the maximum, and multiplying zero by 3.3 is still zero.
inline double adc_to_volts(int raw) {
  return (raw / 4095) * 3.3;
}

// Returns part as a percentage of whole, rounded to the nearest whole number.
// Returns 0 when whole is 0.
//
// BUG: the same division problem, plus nothing protects against whole being 0,
// which is a crash rather than a wrong answer.
inline int percent_of(int part, int whole) {
  return (part / whole) * 100;
}

// Returns the mean of count samples. Returns 0.0 when count is 0.
//
// BUG: the sum of whole numbers divided by a whole number is whole number
// division again, so the fractional part of the average is thrown away.
inline double average(const int* samples, int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    sum += samples[i];
  }
  return sum / count;
}

#endif  // LESSON_SOLUTION_HPP
