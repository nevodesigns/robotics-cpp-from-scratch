// rc/sim/plot.hpp
//
// The mapping from lesson 00-07, graduated.
//
// Turning a trajectory into a picture is two jobs, and only one of them is
// interesting. This is that one: metres into rows and columns. The drawing
// itself, whether into a terminal grid or a Qt widget, is short and rarely
// wrong.
//
// Three decisions here each exist because the obvious alternative produces a
// picture that lies. The rectangle is seeded by the first pose rather than by
// the origin, so a path that never goes near zero is not drawn in a corner. One
// scale serves both axes, so a circle stays a circle and the shape of the path
// remains evidence about the robot. And the row is measured down from the top,
// because a robot's y grows upward and a screen's rows do not.
//
// Phase 10 draws the same mapping into a window. What changes there is the
// surface and the aspect factor, which is one for square pixels.

#ifndef RC_SIM_PLOT_HPP
#define RC_SIM_PLOT_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace sim {

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
inline PlotBounds include(PlotBounds bounds, const Pose& pose) {
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

// ---------------------------------------------------------------------------
// The general form, from lesson 00-08.
//
// A terminal and a window differ in one thing that matters to a mapping: the
// shape of a cell. Making that a parameter is what lets one function serve
// both, and the terminal functions above are this one with the aspect of a
// character and no margin.
//
// Named for the general case rather than taking the plain names the lesson
// uses. Both forms live in this namespace and every caller passes an rc::sim
// type, so argument dependent lookup would find a library `place` beside a
// learner's own `place` and refuse to choose. The lesson keeps the short name,
// because that is the one a beginner reads.
// ---------------------------------------------------------------------------

// Where a pose lands on a surface, in that surface's own units.
//
// Named across and down rather than x and y on purpose. Down is the direction
// the number grows, which is the opposite of the robot's y, and calling it y
// invites exactly the mistake this whole family of functions exists to avoid.
struct Point {
  double across = 0.0;
  double down = 0.0;
};

// One mapping for both surfaces.
//
// The terminal and the window differ in exactly one thing that matters here:
// the shape of a cell. A terminal character is about twice as tall as it is
// wide, so aspect is 2. A pixel is square, so aspect is 1. Everything else,
// the fitting, the centring and the flip, is identical, and discovering that is
// the point of this lesson.
inline double scale_for_surface(const PlotBounds& bounds, double across, double down,
                            double aspect, double margin) {
  const double usable_across = across - 2.0 * margin;
  const double usable_down = down - 2.0 * margin;
  if (usable_across <= 0.0 || usable_down <= 0.0) return 0.0;

  const double width = bounds_width(bounds);
  const double height = bounds_height(bounds);

  // An axis with no extent does not constrain the scale, and dividing by it
  // gives infinity. The same guard as the terminal version, for the same
  // reason: without it a straight path draws nothing at all.
  const double by_across = width > 1e-12 ? usable_across / (width * aspect) : usable_across;
  const double by_down = height > 1e-12 ? usable_down / height : usable_down;

  // One scale, the tighter fit. Two would stretch the path to fill the surface
  // and the shape of it would stop being evidence about the robot.
  return by_across < by_down ? by_across : by_down;
}

inline Point place_on_surface(const Pose& pose, const PlotBounds& bounds,
                   double across, double down, double aspect, double margin) {
  const double scale = scale_for_surface(bounds, across, down, aspect, margin);

  const double drawn_across = bounds_width(bounds) * scale * aspect;
  const double drawn_down = bounds_height(bounds) * scale;

  // Centre whatever room is left over after fitting, so the path sits in the
  // middle rather than against one corner.
  const double offset_across = margin + (across - 2.0 * margin - drawn_across) / 2.0;
  const double offset_down = margin + (down - 2.0 * margin - drawn_down) / 2.0;

  Point point;
  point.across = offset_across + (pose.x - bounds.min_x) * scale * aspect;

  // The flip, in the general form. The distance above the bottom of the path
  // becomes a distance below the top of the surface, and it is the same line
  // whether the surface is made of characters or of pixels.
  point.down = down - offset_down - (pose.y - bounds.min_y) * scale;
  return point;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_PLOT_HPP
