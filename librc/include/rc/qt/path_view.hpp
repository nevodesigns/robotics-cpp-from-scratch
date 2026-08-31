// rc/qt/path_view.hpp
//
// The path view from lesson 10-01, graduated.
//
// A widget that draws where the robot has been. Every later tool in this
// curriculum reuses it, because the cheapest debugger for anything that moves
// is a picture of where it went.
//
// Two thirds of this file is not Qt at all. bounds_of and to_widget are plain
// arithmetic, deliberately kept out of paintEvent so that the mapping from
// robot coordinates to pixels can be tested without a window, a screen or an
// event loop. That separation is the reusable idea here; the painting is the
// easy part.

#ifndef RC_QT_PATH_VIEW_HPP
#define RC_QT_PATH_VIEW_HPP

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QSize>
#include <QWidget>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace qt {

struct PathBounds {
  double min_x = 0.0;
  double max_x = 0.0;
  double min_y = 0.0;
  double max_y = 0.0;
  bool empty = true;

  double width() const { return max_x - min_x; }
  double height() const { return max_y - min_y; }
};

inline PathBounds bounds_of(const std::vector<rc::sim::Pose>& path) {
  PathBounds bounds;
  if (path.empty()) return bounds;

  // Seed from the first point rather than from zero. Seeding from zero would
  // silently include the origin in every bounds, so a path that never comes
  // near it would be drawn small and off to one side.
  bounds.empty = false;
  bounds.min_x = bounds.max_x = path.front().x;
  bounds.min_y = bounds.max_y = path.front().y;

  for (const rc::sim::Pose& pose : path) {
    bounds.min_x = std::min(bounds.min_x, pose.x);
    bounds.max_x = std::max(bounds.max_x, pose.x);
    bounds.min_y = std::min(bounds.min_y, pose.y);
    bounds.max_y = std::max(bounds.max_y, pose.y);
  }
  return bounds;
}

inline QPointF to_widget(const rc::sim::Pose& pose, const PathBounds& bounds,
                         QSize size, double margin) {
  const double usable_width = std::max(1.0, size.width() - 2.0 * margin);
  const double usable_height = std::max(1.0, size.height() - 2.0 * margin);

  // A path one point long, or perfectly straight, has zero extent on an axis.
  // Dividing by it gives infinity and everything downstream becomes NaN, so the
  // degenerate case gets a scale that simply centres the path.
  const double span_x = bounds.width() > 1e-9 ? bounds.width() : 1.0;
  const double span_y = bounds.height() > 1e-9 ? bounds.height() : 1.0;

  // One scale for both axes, the tighter of the two fits, so a circle stays a
  // circle rather than becoming an ellipse when the window is not square.
  const double scale = std::min(usable_width / span_x, usable_height / span_y);

  // Centre whatever room is left over after fitting.
  const double drawn_width = bounds.width() * scale;
  const double drawn_height = bounds.height() * scale;
  const double offset_x = margin + (usable_width - drawn_width) / 2.0;
  const double offset_y = margin + (usable_height - drawn_height) / 2.0;

  const double x = offset_x + (pose.x - bounds.min_x) * scale;

  // The flip. The robot's y grows upward and the widget's grows downward, so
  // the distance above the bottom becomes a distance below the top. Leaving
  // this out draws a mirror image that looks plausible enough to miss.
  const double y = size.height() - offset_y - (pose.y - bounds.min_y) * scale;

  return QPointF(x, y);
}

class PathView : public QWidget {
  Q_OBJECT

 public:
  explicit PathView(QWidget* parent = nullptr) : QWidget(parent) {}

  void setPath(std::vector<rc::sim::Pose> path) {
    path_ = std::move(path);
    update();
  }

  const std::vector<rc::sim::Pose>& path() const { return path_; }

  QColor background() const { return background_; }
  QColor trail() const { return trail_; }

 protected:
  // Qt calls this when the pixels are needed. Painting anywhere else either does
  // nothing or flickers, and it is the first thing to check when a custom widget
  // misbehaves.
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), background_);

    if (path_.size() < 2) return;

    const PathBounds bounds = bounds_of(path_);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(trail_, 2.0));
    for (std::size_t i = 1; i < path_.size(); ++i) {
      painter.drawLine(to_widget(path_[i - 1], bounds, size(), margin_),
                       to_widget(path_[i], bounds, size(), margin_));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(robot_);
    painter.drawEllipse(to_widget(path_.back(), bounds, size(), margin_), 4.0, 4.0);
  }

 private:
  std::vector<rc::sim::Pose> path_;
  QColor background_{20, 24, 22};
  QColor trail_{79, 189, 179};
  QColor robot_{216, 164, 65};
  double margin_ = 12.0;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_PATH_VIEW_HPP
