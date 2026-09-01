// The framework's own main is suppressed, because a widget cannot exist before
// a QApplication does, and that has to be created first.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QSize>

#include <rc/sim/diff_drive.hpp>
#include <rc/sim/plot.hpp>

#include <cmath>
#include <vector>

#include "solution.hpp"

namespace {

using rc::sim::Pose;
using rc::sim::PlotBounds;

constexpr double kTerminalAspect = 2.0;
constexpr double kPixelAspect = 1.0;

Pose at(double x, double y) {
  Pose pose;
  pose.x = x;
  pose.y = y;
  pose.theta = 0.0;
  return pose;
}

PlotBounds bounds_of(const std::vector<Pose>& path) {
  PlotBounds bounds;
  for (const Pose& pose : path) bounds = rc::sim::include(bounds, pose);
  return bounds;
}

std::vector<Pose> unit_circle() {
  std::vector<Pose> path;
  for (int i = 0; i <= 200; ++i) {
    const double angle = static_cast<double>(i) * 0.0314159;
    path.push_back(at(std::cos(angle), std::sin(angle)));
  }
  return path;
}

std::vector<Pose> quarter_circle() {
  std::vector<Pose> path{Pose{}};
  const std::vector<Pose> driven = rc::sim::drive(Pose{}, 0.9, 1.1, 0.3, 0.02, 118);
  path.insert(path.end(), driven.begin(), driven.end());
  return path;
}

// Renders a widget with no window and no screen, which is what lets these run
// on a machine that has no display.
QImage render(PathWindow& window, QSize size) {
  window.resize(size);
  QImage canvas(size, QImage::Format_ARGB32);
  canvas.fill(Qt::transparent);
  window.render(&canvas);
  return canvas;
}

// The bounding box of everything that is not the background.
struct DrawnArea {
  int left = 0, right = -1, top = 0, bottom = -1;
  int count = 0;
  bool any() const { return count > 0; }
  int width() const { return right - left; }
  int height() const { return bottom - top; }
};

DrawnArea drawn_area(const QImage& image, QColor background) {
  DrawnArea area;
  area.left = image.width();
  area.top = image.height();
  area.right = -1;
  area.bottom = -1;

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (pixel == background) continue;
      ++area.count;
      if (x < area.left) area.left = x;
      if (x > area.right) area.right = x;
      if (y < area.top) area.top = y;
      if (y > area.bottom) area.bottom = y;
    }
  }
  return area;
}

}  // namespace

// ---------------------------------------------------------------------------
// The arithmetic, which needs no window at all.
// ---------------------------------------------------------------------------

RC_TEST("with the terminal's cell shape, the mapping agrees with lesson 00-07") {
  // The claim this lesson makes: one mapping serves both surfaces, and the
  // terminal is just the case where a cell is twice as tall as it is wide.
  const std::vector<Pose> path = unit_circle();
  const PlotBounds bounds = bounds_of(path);

  const double columns = 60.0;
  const double rows = 20.0;

  // One difference, and it is the interesting kind. A character grid is indexed
  // by cell, so sixty columns give fifty nine steps between the first centre
  // and the last. A pixel surface is continuous and the whole extent is
  // available. Passing the step count rather than the cell count is what makes
  // the two agree, and it is the difference between a mapping that is nearly
  // right and one that is right.
  const double general =
      surface_scale(bounds, columns - 1.0, rows - 1.0, kTerminalAspect, 0.0);
  const double terminal = rc::sim::scale_to_fit(bounds, 60, 20);

  RC_CHECK_NEAR(general, terminal, 1e-9);

  // And with the cell count instead, it is close but not equal, which is what
  // the off by one looks like when it does not crash anything.
  const double naive = surface_scale(bounds, columns, rows, kTerminalAspect, 0.0);
  RC_CHECK(naive != terminal);
  RC_CHECK(std::fabs(naive - terminal) < 1.0);
}

RC_TEST("a circle is round in pixels, which is a different number from round in characters") {
  const std::vector<Pose> path = unit_circle();
  const PlotBounds bounds = bounds_of(path);

  double left = 1e9, right = -1e9, top = 1e9, bottom = -1e9;
  for (const Pose& pose : path) {
    const Point point = place(pose, bounds, 400.0, 300.0, kPixelAspect, 10.0);
    if (point.across < left) left = point.across;
    if (point.across > right) right = point.across;
    if (point.down < top) top = point.down;
    if (point.down > bottom) bottom = point.down;
  }

  const double drawn_width = right - left;
  const double drawn_height = bottom - top;
  RC_REQUIRE(drawn_height > 0.0);

  // As wide as it is tall. In the terminal the same circle is twice as wide,
  // and both are correct, because the cells are different shapes.
  RC_CHECK_NEAR(drawn_width / drawn_height, 1.0, 0.02);
}

RC_TEST("a point higher up the field is drawn nearer the top, on any surface") {
  PlotBounds bounds;
  bounds = rc::sim::include(bounds, at(0.0, 0.0));
  bounds = rc::sim::include(bounds, at(1.0, 1.0));

  const Point low = place(at(0.0, 0.0), bounds, 400.0, 300.0, kPixelAspect, 10.0);
  const Point high = place(at(1.0, 1.0), bounds, 400.0, 300.0, kPixelAspect, 10.0);

  RC_CHECK(high.down < low.down);
  RC_CHECK(high.across > low.across);
}

RC_TEST("nothing is placed inside the margin") {
  const std::vector<Pose> path = unit_circle();
  const PlotBounds bounds = bounds_of(path);
  const double margin = 25.0;

  for (const Pose& pose : path) {
    const Point point = place(pose, bounds, 400.0, 300.0, kPixelAspect, margin);
    RC_REQUIRE(point.across >= margin - 1e-6);
    RC_REQUIRE(point.across <= 400.0 - margin + 1e-6);
    RC_REQUIRE(point.down >= margin - 1e-6);
    RC_REQUIRE(point.down <= 300.0 - margin + 1e-6);
  }
}

RC_TEST("the path is centred in whatever room is left over") {
  // A wide surface and a tall path: the empty space belongs on both sides
  // equally, not all on one.
  PlotBounds bounds;
  bounds = rc::sim::include(bounds, at(0.0, 0.0));
  bounds = rc::sim::include(bounds, at(0.2, 1.0));

  const Point low = place(at(0.0, 0.0), bounds, 400.0, 200.0, kPixelAspect, 0.0);
  const Point high = place(at(0.2, 1.0), bounds, 400.0, 200.0, kPixelAspect, 0.0);

  const double left_gap = (low.across < high.across ? low.across : high.across);
  const double right_gap = 400.0 - (low.across > high.across ? low.across : high.across);
  RC_CHECK_NEAR(left_gap, right_gap, 1e-6);
}

RC_TEST("a straight path still produces finite coordinates") {
  PlotBounds bounds;
  bounds = rc::sim::include(bounds, at(0.0, 5.0));
  bounds = rc::sim::include(bounds, at(4.0, 5.0));

  const Point point = place(at(2.0, 5.0), bounds, 400.0, 300.0, kPixelAspect, 10.0);
  RC_CHECK(std::isfinite(point.across));
  RC_CHECK(std::isfinite(point.down));
}

RC_TEST("a path that has not moved yet produces finite coordinates") {
  // The case the guard is actually for, and it is narrower than it looks.
  // Taking the smaller of the two candidate scales is already a partial guard:
  // an axis with no extent gives infinity, and infinity is never the smaller.
  // What defeats that is a path where *every* axis is degenerate, which is
  // every path on its first frame, before anything has moved.
  PlotBounds bounds;
  bounds = rc::sim::include(bounds, at(2.0, 3.0));

  const double scale = surface_scale(bounds, 400.0, 300.0, kPixelAspect, 10.0);
  RC_REQUIRE(std::isfinite(scale));

  const Point point = place(at(2.0, 3.0), bounds, 400.0, 300.0, kPixelAspect, 10.0);
  RC_CHECK(std::isfinite(point.across));
  RC_CHECK(std::isfinite(point.down));
}

RC_TEST("a window given a path that has not moved draws without crashing") {
  PathWindow window;
  window.setPath(std::vector<Pose>(8, at(1.0, 1.0)));
  const QImage canvas = render(window, QSize(200, 150));
  RC_CHECK_EQ(canvas.width(), 200);   // it rendered at all
}

RC_TEST("a surface smaller than its own margins does not produce nonsense") {
  PlotBounds bounds;
  bounds = rc::sim::include(bounds, at(0.0, 0.0));
  bounds = rc::sim::include(bounds, at(1.0, 1.0));

  const double scale = surface_scale(bounds, 10.0, 10.0, kPixelAspect, 20.0);
  RC_CHECK(std::isfinite(scale));
  RC_CHECK_NEAR(scale, 0.0, 1e-12);
}

// ---------------------------------------------------------------------------
// The window, rendered with no screen.
// ---------------------------------------------------------------------------

RC_TEST("an empty window draws only its background") {
  PathWindow window;
  const QImage canvas = render(window, QSize(200, 150));
  const DrawnArea area = drawn_area(canvas, QColor(20, 24, 22));
  RC_CHECK(!area.any());
}

RC_TEST("the path is actually drawn") {
  PathWindow window;
  window.setPath(quarter_circle());
  const QImage canvas = render(window, QSize(320, 240));
  const DrawnArea area = drawn_area(canvas, QColor(20, 24, 22));

  RC_REQUIRE(area.any());
  RC_CHECK(area.count > 100);
}

RC_TEST("the drawing respects the margin the window asked for") {
  PathWindow window;
  window.setPath(quarter_circle());
  const QImage canvas = render(window, QSize(320, 240));
  const DrawnArea area = drawn_area(canvas, QColor(20, 24, 22));
  RC_REQUIRE(area.any());

  // A few pixels of slack for the pen width and the antialiasing, which spread
  // a line either side of where the arithmetic put it.
  const int slack = 5;
  const int margin = static_cast<int>(window.margin());
  RC_CHECK(area.left >= margin - slack);
  RC_CHECK(area.top >= margin - slack);
  RC_CHECK(area.right <= canvas.width() - margin + slack);
  RC_CHECK(area.bottom <= canvas.height() - margin + slack);
}

RC_TEST("a circle drawn in the window comes out round") {
  // The same check as the terminal lesson, on the other surface, and the number
  // it expects is different for a reason a learner can now name.
  PathWindow window;
  window.setPath(unit_circle());
  const QImage canvas = render(window, QSize(400, 300));
  const DrawnArea area = drawn_area(canvas, QColor(20, 24, 22));

  RC_REQUIRE(area.any());
  RC_REQUIRE(area.height() > 0);
  RC_CHECK_NEAR(static_cast<double>(area.width()) / static_cast<double>(area.height()),
                1.0, 0.08);
}

RC_TEST("the picture fills the window rather than sitting in a corner") {
  PathWindow window;
  window.setPath(quarter_circle());
  const QImage canvas = render(window, QSize(400, 300));
  const DrawnArea area = drawn_area(canvas, QColor(20, 24, 22));

  RC_REQUIRE(area.any());
  RC_CHECK(area.height() > canvas.height() / 2);
}

int main(int argc, char** argv) {
  // With QT_QPA_PLATFORM=offscreen this needs no display, which is how these
  // run in continuous integration and on a machine with no desktop at all.
  QApplication application(argc, argv);
  return rc::test::run_all();
}
