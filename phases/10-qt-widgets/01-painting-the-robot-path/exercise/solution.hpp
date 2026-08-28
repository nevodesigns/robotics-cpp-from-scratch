#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
#include <QSize>
#include <QWidget>

#include <algorithm>
#include <vector>

#include <rc/sim/diff_drive.hpp>

// The extent of a path in the robot's own units, metres.
struct PathBounds {
  double min_x = 0.0;
  double max_x = 0.0;
  double min_y = 0.0;
  double max_y = 0.0;
  bool empty = true;

  double width() const { return max_x - min_x; }
  double height() const { return max_y - min_y; }
};

// The smallest and largest x and y across the path.
// An empty path gives a bounds with empty set to true.
inline PathBounds bounds_of(const std::vector<rc::sim::Pose>& path) {
  // TODO
  (void)path;
  return PathBounds{};
}

// Maps a pose in metres to a pixel position inside a widget of this size.
//
//   fit the bounds into the widget, minus the margin on every side
//   use one scale for both axes, the smaller of the two fits, so the shape is
//     not distorted
//   flip y, because the robot's y grows upward and the widget's grows downward
//   a degenerate bounds, zero width or height, must not produce NaN
inline QPointF to_widget(const rc::sim::Pose& pose, const PathBounds& bounds,
                         QSize size, double margin) {
  // TODO
  (void)pose;
  (void)bounds;
  (void)size;
  (void)margin;
  return QPointF(0.0, 0.0);
}

class PathView : public QWidget {
  Q_OBJECT

 public:
  explicit PathView(QWidget* parent = nullptr) : QWidget(parent) {}

  void setPath(std::vector<rc::sim::Pose> path) {
    path_ = std::move(path);
    update();   // asks for a repaint, does not paint
  }

  const std::vector<rc::sim::Pose>& path() const { return path_; }

  QColor background() const { return background_; }
  QColor trail() const { return trail_; }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);

    // TODO
    // 1. fill the whole widget with background_
    // 2. if there are at least two poses, draw the path as a connected line in
    //    trail_, using to_widget for each point
    // 3. mark the final pose with a filled circle in robot_
    (void)painter;
  }

 private:
  std::vector<rc::sim::Pose> path_;
  QColor background_{20, 24, 22};
  QColor trail_{79, 189, 179};
  QColor robot_{216, 164, 65};
  double margin_ = 12.0;
};

#endif  // LESSON_SOLUTION_HPP
