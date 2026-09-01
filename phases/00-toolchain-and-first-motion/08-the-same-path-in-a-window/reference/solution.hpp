#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QWidget>

#include <rc/sim/diff_drive.hpp>
#include <rc/sim/plot.hpp>

#include <vector>

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
inline double surface_scale(const rc::sim::PlotBounds& bounds, double across, double down,
                            double aspect, double margin) {
  const double usable_across = across - 2.0 * margin;
  const double usable_down = down - 2.0 * margin;
  if (usable_across <= 0.0 || usable_down <= 0.0) return 0.0;

  const double width = rc::sim::bounds_width(bounds);
  const double height = rc::sim::bounds_height(bounds);

  // An axis with no extent does not constrain the scale, and dividing by it
  // gives infinity. The same guard as the terminal version, for the same
  // reason: without it a straight path draws nothing at all.
  const double by_across = width > 1e-12 ? usable_across / (width * aspect) : usable_across;
  const double by_down = height > 1e-12 ? usable_down / height : usable_down;

  // One scale, the tighter fit. Two would stretch the path to fill the surface
  // and the shape of it would stop being evidence about the robot.
  return by_across < by_down ? by_across : by_down;
}

inline Point place(const rc::sim::Pose& pose, const rc::sim::PlotBounds& bounds,
                   double across, double down, double aspect, double margin) {
  const double scale = surface_scale(bounds, across, down, aspect, margin);

  const double drawn_across = rc::sim::bounds_width(bounds) * scale * aspect;
  const double drawn_down = rc::sim::bounds_height(bounds) * scale;

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

// ---------------------------------------------------------------------------
// The window, supplied. You are not expected to write Qt yet: phase 09 is where
// that starts. What is worth reading is how little of this file is Qt, and that
// the only line doing any thinking calls the function you wrote.
// ---------------------------------------------------------------------------

class PathWindow : public QWidget {
  // Every Qt class that will ever declare a signal or a slot starts with this,
  // and this one has neither yet, so removing it here would change nothing.
  // It is here because it is what a widget is written with, and lesson 09-02 is
  // about what it actually does.
  Q_OBJECT

 public:
  explicit PathWindow(QWidget* parent = nullptr) : QWidget(parent) {}

  void setPath(std::vector<rc::sim::Pose> path) {
    path_ = std::move(path);
    update();   // asks Qt to repaint when it next can
  }

  const std::vector<rc::sim::Pose>& path() const { return path_; }
  double margin() const { return margin_; }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), background_);
    if (path_.size() < 2) return;

    rc::sim::PlotBounds bounds;
    for (const rc::sim::Pose& pose : path_) bounds = rc::sim::include(bounds, pose);

    const double across = static_cast<double>(width());
    const double down = static_cast<double>(height());

    // A pixel is square, so the aspect is one. That single argument is the
    // whole difference between this and the terminal.
    const double aspect = 1.0;

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(trail_, 2.0));
    for (std::size_t i = 1; i < path_.size(); ++i) {
      const Point from = place(path_[i - 1], bounds, across, down, aspect, margin_);
      const Point to = place(path_[i], bounds, across, down, aspect, margin_);
      painter.drawLine(QPointF(from.across, from.down), QPointF(to.across, to.down));
    }

    const Point last = place(path_.back(), bounds, across, down, aspect, margin_);
    painter.setPen(Qt::NoPen);
    painter.setBrush(robot_);
    painter.drawEllipse(QPointF(last.across, last.down), 4.0, 4.0);
  }

 private:
  std::vector<rc::sim::Pose> path_;
  QColor background_{20, 24, 22};
  QColor trail_{79, 189, 179};
  QColor robot_{216, 164, 65};
  double margin_ = 12.0;
};

#endif  // LESSON_SOLUTION_HPP
