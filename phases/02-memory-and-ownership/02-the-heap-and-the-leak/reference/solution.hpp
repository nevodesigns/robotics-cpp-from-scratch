#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

struct Sample {
  double value = 0.0;
  bool valid = true;
};

inline double average_of_readings(const double* source, int count) {
  // Answer the impossible case before allocating anything. Nothing acquired
  // means nothing to release, and the early return becomes harmless.
  if (count <= 0) return 0.0;

  double* scratch = new double[count];

  double sum = 0.0;
  for (int i = 0; i < count; ++i) {
    scratch[i] = source[i];
    sum += scratch[i];
  }

  delete[] scratch;
  return sum / count;
}

inline int count_above(const double* source, int count, double limit, int give_up_after) {
  if (count <= 0) return 0;

  double* scratch = new double[count];

  int found = 0;
  for (int i = 0; i < count; ++i) {
    scratch[i] = source[i];
    if (scratch[i] > limit) {
      ++found;
      if (found > give_up_after) break;   // leave the loop, not the function
    }
  }

  // One way out, so one delete. Turning an early return into a break is the
  // cheapest fix available, and it is the one that survives somebody adding a
  // third exit condition later.
  delete[] scratch;
  return found;
}

inline Sample first_valid_or_default(const Sample* source, int count) {
  if (count <= 0) return Sample{};

  Sample* copies = new Sample[count];

  Sample answer{};
  for (int i = 0; i < count; ++i) {
    copies[i] = source[i];
    if (copies[i].valid) {
      answer = copies[i];
      break;
    }
  }

  // new[] pairs with delete[]. Using plain delete on an array is undefined
  // behaviour, and it frequently does not crash, which is worse than if it did.
  delete[] copies;
  return answer;
}

#endif  // LESSON_SOLUTION_HPP
