// The test framework's own main is suppressed here, because a widget needs a
// QApplication to exist before it does, and that has to be created first.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QApplication>
#include <QImage>
#include <QSize>

#include <cmath>
#include <vector>

#include "solution.hpp"

namespace {

std::vector<rc::sim::Pose> quarter_circle() {
  // The same drive as lesson 00-05, through the same function, now graduated
  // into rc::sim. Nothing about it was changed to make it drawable.
  return rc::sim::drive(rc::sim::Pose{}, 0.55, 0.75, 0.30, 0.05, 47);
}

// Renders a widget with no window and no screen at all, which is what lets
// these tests run on a machine that has no display.
QImage render(PathView& view, QSize size) {
  view.resize(size);
  QImage canvas(size, QImage::Format_ARGB32);
  canvas.fill(Qt::transparent);
  view.render(&canvas);
  return canvas;
}

int pixels_matching(const QImage& image, QColor colour) {
  int count = 0;
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x)
      if (image.pixelColor(x, y) == colour) ++count;
  return count;
}

}  // namespace

RC_TEST("the bounds of an empty path are empty") {
  const PathBounds bounds = bounds_of({});
  RC_CHECK(bounds.empty);
}

RC_TEST("bounds cover every point") {
  const std::vector<rc::sim::Pose> path = {{0.0, 0.0, 0.0}, {2.0, -1.0, 0.0}, {-1.0, 3.0, 0.0}};
  const PathBounds bounds = bounds_of(path);
  RC_CHECK(!bounds.empty);
  RC_CHECK_NEAR(bounds.min_x, -1.0, 1e-9);
  RC_CHECK_NEAR(bounds.max_x, 2.0, 1e-9);
  RC_CHECK_NEAR(bounds.min_y, -1.0, 1e-9);
  RC_CHECK_NEAR(bounds.max_y, 3.0, 1e-9);
}

RC_TEST("bounds are seeded from the path, not from the origin") {
  // A path far from zero must not have the origin dragged into its bounds, or
  // it would be drawn tiny and pushed into a corner.
  const std::vector<rc::sim::Pose> path = {{10.0, 10.0, 0.0}, {11.0, 12.0, 0.0}};
  const PathBounds bounds = bounds_of(path);
  RC_CHECK_NEAR(bounds.min_x, 10.0, 1e-9);
  RC_CHECK_NEAR(bounds.min_y, 10.0, 1e-9);
}

RC_TEST("a mapped point lands inside the widget") {
  const std::vector<rc::sim::Pose> path = quarter_circle();
  const PathBounds bounds = bounds_of(path);
  const QSize size(240, 180);
  for (const rc::sim::Pose& pose : path) {
    const QPointF point = to_widget(pose, bounds, size, 12.0);
    RC_REQUIRE(!std::isnan(point.x()));
    RC_CHECK(point.x() >= 0.0 && point.x() <= size.width());
    RC_CHECK(point.y() >= 0.0 && point.y() <= size.height());
  }
}

RC_TEST("y is flipped, so a higher pose draws further up the widget") {
  // This is the check that catches a missing flip, and it catches nothing else.
  const std::vector<rc::sim::Pose> path = {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}};
  const PathBounds bounds = bounds_of(path);
  const QSize size(200, 200);

  const QPointF low = to_widget(path[0], bounds, size, 10.0);
  const QPointF high = to_widget(path[1], bounds, size, 10.0);

  RC_CHECK(high.y() < low.y());   // greater y in metres means fewer pixels down
  RC_CHECK(high.x() > low.x());   // and x is not flipped
}

RC_TEST("a degenerate path does not produce NaN") {
  const std::vector<rc::sim::Pose> single = {{4.0, 4.0, 0.0}};
  const PathBounds bounds = bounds_of(single);
  const QPointF point = to_widget(single[0], bounds, QSize(100, 100), 10.0);
  RC_CHECK(!std::isnan(point.x()));
  RC_CHECK(!std::isnan(point.y()));
}

RC_TEST("a straight horizontal path does not produce NaN") {
  const std::vector<rc::sim::Pose> flat = {{0.0, 1.0, 0.0}, {5.0, 1.0, 0.0}};
  const PathBounds bounds = bounds_of(flat);
  const QPointF point = to_widget(flat[1], bounds, QSize(100, 100), 10.0);
  RC_CHECK(!std::isnan(point.y()));
}

RC_TEST("the widget fills its background") {
  PathView view;
  const QImage canvas = render(view, QSize(80, 60));
  // Every pixel is painted, so none of the transparent fill survives.
  RC_CHECK_EQ(pixels_matching(canvas, QColor(0, 0, 0, 0)), 0);
  RC_CHECK(pixels_matching(canvas, view.background()) > 0);
}

RC_TEST("an empty view draws background and nothing else") {
  PathView view;
  const QImage canvas = render(view, QSize(80, 60));
  RC_CHECK_EQ(pixels_matching(canvas, view.background()), 80 * 60);
}

RC_TEST("a path puts ink on the widget") {
  PathView view;
  view.setPath(quarter_circle());
  const QImage canvas = render(view, QSize(240, 180));

  const int background = pixels_matching(canvas, view.background());
  RC_CHECK(background < 240 * 180);        // something was drawn
  RC_CHECK(background > (240 * 180) / 2);  // and it is a path, not a filled rectangle
}

RC_TEST("the drawn path spans the widget rather than sitting in a corner") {
  PathView view;
  view.setPath(quarter_circle());
  const QImage canvas = render(view, QSize(240, 180));

  int leftmost = canvas.width();
  int rightmost = -1;
  for (int y = 0; y < canvas.height(); ++y) {
    for (int x = 0; x < canvas.width(); ++x) {
      if (canvas.pixelColor(x, y) == view.background()) continue;
      leftmost = std::min(leftmost, x);
      rightmost = std::max(rightmost, x);
    }
  }
  RC_REQUIRE(rightmost > 0);
  // The fit should use most of the available width.
  RC_CHECK(rightmost - leftmost > canvas.width() / 2);
}

int main(int argc, char** argv) {
  // A widget cannot exist before this. With QT_QPA_PLATFORM=offscreen it needs
  // no display, which is how these tests run in continuous integration.
  QApplication application(argc, argv);
  return rc::test::run_all();
}
