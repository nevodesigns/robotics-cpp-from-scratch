#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Each function below allocates, and each one has at least one way out that
// skips the cleanup. Fix all three so nothing is left allocated on any path,
// without changing what they return.
//
// This is the one lesson in the curriculum where you write new and delete by
// hand. Lesson 02-03 shows you what replaces them.

struct Sample {
  double value = 0.0;
  bool valid = true;
};

// The mean of count values read from source, or 0.0 when count is not positive.
inline double average_of_readings(const double* source, int count) {
  double* scratch = new double[count > 0 ? count : 1];

  if (count <= 0) {
    return 0.0;   // BUG: scratch is never given back
  }

  double sum = 0.0;
  for (int i = 0; i < count; ++i) {
    scratch[i] = source[i];
    sum += scratch[i];
  }

  delete[] scratch;
  return sum / count;
}

// How many values are above limit, stopping as soon as more than give_up_after
// have been found, because the caller only wanted to know whether there were
// many of them.
inline int count_above(const double* source, int count, double limit, int give_up_after) {
  double* scratch = new double[count > 0 ? count : 1];

  int found = 0;
  for (int i = 0; i < count; ++i) {
    scratch[i] = source[i];
    if (scratch[i] > limit) {
      ++found;
      if (found > give_up_after) {
        return found;   // BUG: the early way out skips the cleanup
      }
    }
  }

  delete[] scratch;
  return found;
}

// The first sample marked valid, or a default constructed Sample when there is
// none.
inline Sample first_valid_or_default(const Sample* source, int count) {
  Sample* copies = new Sample[count > 0 ? count : 1];

  for (int i = 0; i < count; ++i) {
    copies[i] = source[i];
    if (copies[i].valid) {
      Sample answer = copies[i];
      delete copies;   // BUG: allocated with new[], given back with plain delete
      return answer;
    }
  }

  delete[] copies;
  return Sample{};
}

#endif  // LESSON_SOLUTION_HPP
