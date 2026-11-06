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
  const char* names[3] = {"left speed", "right speed", "duration"};
  const double low[3] = {0.0, 0.0, 0.1};
  const double high[3] = {2.0, 2.0, 60.0};
  double value[3] = {0.0, 0.0, 0.0};

  // Check everything before doing anything: a bad third number found after the picture was drawn
  // leaves half an answer on the screen.
  for (int i = 0; i < 3; ++i) {
    const rc::core::Asked asked = rc::core::ask(in, low[i], high[i], 1);
    if (!asked.entry.ok) {
      err << names[i] << ": " << asked.entry.problem << " (allowed " << low[i] << " to " << high[i] << ")\n";
      return rc::core::status_for_misuse();
    }
    value[i] = asked.entry.value;
  }

  const double wheel_base = 0.5;
  int steps = static_cast<int>(value[2] / 0.01 + 0.5);
  if (steps < 1) steps = 1;
  const double dt = value[2] / steps;

  // First pass: where does the path go, so the picture can be scaled to fit it.
  rc::sim::Pose pose;
  rc::plot::PlotBounds bounds = rc::plot::include(rc::plot::PlotBounds{}, pose);
  for (int i = 0; i < steps; ++i) {
    pose = rc::sim::step_midpoint(pose, value[0], value[1], wheel_base, dt);
    bounds = rc::plot::include(bounds, pose);
  }
  const rc::sim::Pose end = pose;

  const int kColumns = 40;
  const int kRows = 12;
  const double scale = rc::plot::scale_to_fit(bounds, kColumns, kRows);
  char grid[kRows][kColumns + 1];
  for (int r = 0; r < kRows; ++r) {
    for (int c = 0; c < kColumns; ++c) grid[r][c] = ' ';
    grid[r][kColumns] = '\0';
  }

  // Second pass: the same path again, marking the grid.
  pose = rc::sim::Pose{};
  for (int i = 0; i <= steps; ++i) {
    const int c = rc::plot::column_for(pose.x, bounds, scale);
    const int r = rc::plot::row_for(pose.y, bounds, scale, kRows);
    if (c >= 0 && c < kColumns && r >= 0 && r < kRows) grid[r][c] = '*';
    if (i < steps) pose = rc::sim::step_midpoint(pose, value[0], value[1], wheel_base, dt);
  }
  const int sc = rc::plot::column_for(0.0, bounds, scale);
  const int sr = rc::plot::row_for(0.0, bounds, scale, kRows);
  const int ec = rc::plot::column_for(end.x, bounds, scale);
  const int er = rc::plot::row_for(end.y, bounds, scale, kRows);
  if (sc >= 0 && sc < kColumns && sr >= 0 && sr < kRows) grid[sr][sc] = 'S';
  if (ec >= 0 && ec < kColumns && er >= 0 && er < kRows) grid[er][ec] = 'E';

  for (int r = 0; r < kRows; ++r) {
    int last = kColumns - 1;
    while (last >= 0 && grid[r][last] == ' ') --last;
    grid[r][last + 1] = '\0';
    out << grid[r] << "\n";
  }

  const double degrees = end.theta * 180.0 / 3.14159265358979323846;
  out << std::fixed << std::setprecision(3) << "end x=" << end.x << " y=" << end.y << std::setprecision(1)
      << " heading=" << degrees << "\n";
  return rc::core::kSuccess;
}

#endif  // LESSON_SOLUTION_HPP
