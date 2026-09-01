#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

// A terminal character is roughly twice as tall as it is wide. Ignoring that
// draws every circle as an ellipse, and it is the reason a plot of a robot
// turning in a perfect circle looks like it is drifting sideways.
inline double character_aspect() { return 2.0; }

// The rectangle a path occupies, in metres. Built up one pose at a time,
// because at this point in the curriculum there is nowhere to put a list of
// them yet.
struct PlotBounds {
  double min_x = 0.0;
  double max_x = 0.0;
  double min_y = 0.0;
  double max_y = 0.0;
  bool empty = true;
};

// Grows the rectangle to contain one more pose.
//
// The first pose seeds it rather than being compared against zero. Starting
// from zero silently includes the origin in every plot, so a path that never
// goes near it is drawn small and pushed into a corner.
inline PlotBounds include(PlotBounds bounds, const rc::sim::Pose& pose) {
  if (bounds.empty) {
    bounds.empty = false;
    bounds.min_x = bounds.max_x = pose.x;
    bounds.min_y = bounds.max_y = pose.y;
    return bounds;
  }

  if (pose.x < bounds.min_x) bounds.min_x = pose.x;
  if (pose.x > bounds.max_x) bounds.max_x = pose.x;
  if (pose.y < bounds.min_y) bounds.min_y = pose.y;
  if (pose.y > bounds.max_y) bounds.max_y = pose.y;
  return bounds;
}

inline double bounds_width(const PlotBounds& bounds) {
  return bounds.empty ? 0.0 : bounds.max_x - bounds.min_x;
}

inline double bounds_height(const PlotBounds& bounds) {
  return bounds.empty ? 0.0 : bounds.max_y - bounds.min_y;
}

// How many rows of characters one metre becomes, chosen so the whole path fits.
//
// One number for both axes, not one each. Two scales stretch the picture to
// fill the grid, and then a circle is an ellipse and a square is a rectangle,
// and the shape of the path stops being evidence about the robot.
inline double scale_to_fit(const PlotBounds& bounds, int columns, int rows) {
  if (columns < 2 || rows < 2) return 0.0;

  const double width = bounds_width(bounds);
  const double height = bounds_height(bounds);

  // A path along one axis has no extent on the other. Dividing by that gives
  // infinity, every coordinate becomes not a number, and the picture is blank
  // with no explanation.
  const double usable_rows = static_cast<double>(rows - 1);
  const double usable_columns = static_cast<double>(columns - 1);

  const double by_height = height > 1e-12 ? usable_rows / height : usable_rows;
  const double by_width =
      width > 1e-12 ? usable_columns / (width * character_aspect()) : usable_columns;

  return by_height < by_width ? by_height : by_width;
}

inline int column_for(double x, const PlotBounds& bounds, double scale) {
  const double offset = (x - bounds.min_x) * scale * character_aspect();
  return static_cast<int>(offset + 0.5);
}

// The flip. A robot's y grows upward and a screen's rows grow downward, so the
// distance above the bottom of the path becomes a distance below the top of the
// grid. Leaving this out draws a mirror image, which looks plausible enough to
// go unnoticed for a long time.
inline int row_for(double y, const PlotBounds& bounds, double scale, int rows) {
  const double from_bottom = (y - bounds.min_y) * scale;
  return (rows - 1) - static_cast<int>(from_bottom + 0.5);
}

#endif  // LESSON_SOLUTION_HPP
