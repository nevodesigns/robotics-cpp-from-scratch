#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <iomanip>
#include <istream>
#include <ostream>

#include <rc/core/ask.hpp>
#include <rc/core/exit_status.hpp>
#include <rc/plot/path.hpp>
#include <rc/sim/arc.hpp>

// The whole of phase 00 as one program's worth of logic: ask for three numbers, check them, drive the
// robot, draw where it went, and say how it ended.
//
// Input and output are parameters and not std::cin and std::cout, so the test can hand it text and read back
// what it wrote. A function that cannot be given its streams cannot be tested without a person.
//
// Returns the status main should return: zero when it worked, the "misused" status when the numbers were bad.
inline int run_drive(std::istream& in, std::ostream& out, std::ostream& err) {
  // TODO: five parts, each from an earlier lesson.
  //
  // 1. Ask for three numbers with rc::core::ask(in, low, high, 1): the left speed from 0 to 2, the
  //    right speed from 0 to 2 and the duration from 0.1 to 60 seconds. Check all three BEFORE doing
  //    anything else. If one is refused, write "<name>: <problem> (allowed <low> to <high>)" and a
  //    newline to err, and return rc::core::status_for_misuse(). Write nothing to out.
  //
  // 2. Drive with rc::sim::step_midpoint on a wheel base of 0.5, in steps of about 0.01 seconds:
  //    steps = duration / 0.01 rounded, at least 1, and dt = duration / steps.
  //
  // 3. Work out the bounds of the path (rc::plot::include) and the scale (rc::plot::scale_to_fit)
  //    for a grid of 40 columns and 12 rows.
  //
  // 4. Draw it into out, one line per row with the spaces at the end of the line taken off: '*' at
  //    every pose (rc::plot::column_for and row_for), then 'S' at the start and 'E' at the end.
  //
  // 5. Finish with one line: end x=<x> y=<y> heading=<degrees>, with x and y to 3 decimal places
  //    and the heading to 1, then return rc::core::kSuccess.
  (void)in;
  (void)out;
  (void)err;
  return rc::core::kFailed;
}

#endif  // LESSON_SOLUTION_HPP
