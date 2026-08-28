#include "solution.hpp"

double celsius_from_raw(int raw) {
  return raw * 0.0625;
}

// Built on top of celsius_from_raw rather than repeating 0.0625. If the sensor
// is ever swapped for one with a different scale, exactly one line changes.
bool is_overheating(int raw) {
  return celsius_from_raw(raw) > 80.0;
}
