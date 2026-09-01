// A project that is not yours, using your package the way anybody would: by
// asking for it by name and linking to it. Nothing here knows where it lives.
#include <steplib/step.hpp>

#include <cmath>
#include <iostream>

int main() {
  if (std::fabs(steplib::step_toward(0.95, 1.0, 0.1) - 1.0) > 1e-9) return 1;
  if (std::fabs(steplib::step_toward(0.0, 100.0, 0.1) - 0.1) > 1e-9) return 1;
  if (std::fabs(steplib::step_toward(0.0, -100.0, 0.1) + 0.1) > 1e-9) return 1;

  std::cout << "consumed steplib\n";
  return 0;
}
