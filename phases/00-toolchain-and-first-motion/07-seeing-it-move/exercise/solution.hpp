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
  // TODO: grow the rectangle to contain this pose.
  //
  // The first pose seeds it rather than being compared against zero. Starting
  // from zero silently includes the origin in every plot, so a path that never
  // goes near it is drawn small and pushed into a corner.
  (void)pose;
  return bounds;
}

inline double bounds_width(const PlotBounds& bounds) {
  // TODO: how wide the rectangle is, and nothing at all when it is empty.
  (void)bounds;
  return 0.0;
}

inline double bounds_height(const PlotBounds& bounds) {
  // TODO: how tall it is.
  (void)bounds;
  return 0.0;
}

// How many rows of characters one metre becomes, chosen so the whole path fits.
//
// One number for both axes, not one each. Two scales stretch the picture to
// fill the grid, and then a circle is an ellipse and a square is a rectangle,
// and the shape of the path stops being evidence about the robot.
inline double scale_to_fit(const PlotBounds& bounds, int columns, int rows) {
  // TODO: how many rows of characters one metre becomes, so the whole path fits.
  //
  // One number for both axes, not one each. Two scales stretch the picture to
  // fill the grid, and then a circle is an ellipse and the shape of the path
  // has stopped being evidence about the robot.
  //
  // Work out the scale each axis could allow and take the smaller. Remember
  // that a character is about twice as tall as it is wide, so a metre across
  // costs twice as many columns as a metre up costs rows.
  //
  // A path along one axis has no extent on the other. Dividing by that gives
  // infinity, every coordinate becomes not a number, and the picture comes out
  // blank with nothing to say why.
  (void)bounds;
  (void)columns;
  (void)rows;
  return 0.0;
}

inline int column_for(double x, const PlotBounds& bounds, double scale) {
  // TODO: which column this x falls in, allowing for the character shape.
  (void)x;
  (void)bounds;
  (void)scale;
  return 0;
}

// The flip. A robot's y grows upward and a screen's rows grow downward, so the
// distance above the bottom of the path becomes a distance below the top of the
// grid. Leaving this out draws a mirror image, which looks plausible enough to
// go unnoticed for a long time.
inline int row_for(double y, const PlotBounds& bounds, double scale, int rows) {
  // TODO: which row this y falls in.
  //
  // The flip. A robot's y grows upward and a screen's rows grow downward, so
  // the distance above the bottom of the path becomes a distance below the top
  // of the grid. Leaving this out draws a mirror image, which looks plausible
  // enough to go unnoticed for a long time.
  (void)y;
  (void)bounds;
  (void)scale;
  (void)rows;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
