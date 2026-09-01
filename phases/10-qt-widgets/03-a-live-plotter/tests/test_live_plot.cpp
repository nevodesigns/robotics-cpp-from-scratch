// A widget cannot exist before a QApplication does, so the framework's own main
// is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QSize>

#include <cmath>
#include <vector>

#include "solution.hpp"

namespace {

const QColor kBackground{20, 24, 22};

QImage render(LivePlot& plot, QSize size) {
  plot.resize(size);
  QImage canvas(size, QImage::Format_ARGB32);
  canvas.fill(Qt::transparent);
  plot.render(&canvas);
  return canvas;
}

struct DrawnArea {
  int top = 0, bottom = -1, count = 0;
  bool any() const { return count > 0; }
  int height() const { return bottom - top; }
};

DrawnArea drawn_area(const QImage& image) {
  DrawnArea area;
  area.top = image.height();
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (image.pixelColor(x, y) == kBackground) continue;
      ++area.count;
      if (y < area.top) area.top = y;
      if (y > area.bottom) area.bottom = y;
    }
  }
  return area;
}

void feed_steady(LivePlot& plot, int samples) {
  for (int i = 0; i < samples; ++i) {
    const double wobble = static_cast<double>((i * 7919) % 3 - 1) * 1e-6;
    plot.addSample(static_cast<double>(i) * 0.05, 12.0 + wobble);
  }
}

}  // namespace

RC_TEST("adding a sample never asks for a repaint") {
  // The check that catches a widget repainting at the data rate. A sensor at a
  // kilohertz would ask for a thousand repaints a second, and a screen shows
  // sixty.
  LivePlot plot;
  for (int i = 0; i < 5000; ++i) plot.addSample(static_cast<double>(i) * 0.001, 1.0);

  RC_CHECK_EQ(plot.repaintRequests(), 0);
  RC_CHECK(plot.stale());

  // Five thousand samples into a widget whose default capacity is four
  // thousand. The guard holds, which is the point of having one: a signal
  // arriving faster than anybody planned cannot grow this without limit.
  RC_CHECK_EQ(plot.series().size(), static_cast<std::size_t>(4000));
}

RC_TEST("refreshing asks for exactly one repaint, however many samples arrived") {
  LivePlot plot;
  for (int i = 0; i < 5000; ++i) plot.addSample(static_cast<double>(i) * 0.001, 1.0);

  plot.refresh();
  RC_CHECK_EQ(plot.repaintRequests(), 1);
  RC_CHECK(!plot.stale());
}

RC_TEST("refreshing with nothing new asks for nothing") {
  // A frame timer runs whether or not data arrived. A chart that repaints on
  // every tick regardless burns a core drawing the same picture.
  LivePlot plot;
  plot.addSample(0.0, 1.0);
  plot.refresh();

  for (int i = 0; i < 100; ++i) plot.refresh();
  RC_CHECK_EQ(plot.repaintRequests(), 1);
}

RC_TEST("a sample after a refresh makes the picture stale again") {
  LivePlot plot;
  plot.addSample(0.0, 1.0);
  plot.refresh();
  RC_CHECK(!plot.stale());

  plot.addSample(0.1, 2.0);
  RC_CHECK(plot.stale());
  plot.refresh();
  RC_CHECK_EQ(plot.repaintRequests(), 2);
}

RC_TEST("the widget draws the samples it was given") {
  LivePlot plot;
  for (int i = 0; i < 100; ++i)
    plot.addSample(static_cast<double>(i) * 0.05, std::sin(static_cast<double>(i) * 0.1));

  const QImage canvas = render(plot, QSize(320, 200));
  const DrawnArea area = drawn_area(canvas);
  RC_REQUIRE(area.any());
  RC_CHECK(area.count > 100);
  RC_CHECK_EQ(plot.paintCount(), 1);
}

RC_TEST("an empty widget draws only its background") {
  LivePlot plot;
  const QImage canvas = render(plot, QSize(200, 120));
  RC_CHECK(!drawn_area(canvas).any());
}

RC_TEST("a single sample is not enough to draw a line, and does not crash") {
  LivePlot plot;
  plot.addSample(0.0, 5.0);
  const QImage canvas = render(plot, QSize(200, 120));
  RC_CHECK(!drawn_area(canvas).any());
}

RC_TEST("a steady signal is drawn as a flat line, not as noise") {
  // The same measurement as lesson 10-02, on the other surface. Without a floor
  // under the axis span the last digit of the reading fills the widget.
  LivePlot plot;
  plot.setMinimumSpan(0.1);
  feed_steady(plot, 200);

  const QImage canvas = render(plot, QSize(320, 200));
  const DrawnArea area = drawn_area(canvas);
  RC_REQUIRE(area.any());

  // A flat trace, a couple of pixels of pen width and antialiasing aside.
  RC_CHECK(area.height() < 12);
}

RC_TEST("a real change still fills a useful part of the widget") {
  LivePlot plot;
  plot.setMinimumSpan(0.1);
  for (int i = 0; i < 100; ++i) plot.addSample(static_cast<double>(i) * 0.05, 12.0);
  for (int i = 100; i < 200; ++i) plot.addSample(static_cast<double>(i) * 0.05, 11.7);

  const QImage canvas = render(plot, QSize(320, 200));
  const DrawnArea area = drawn_area(canvas);
  RC_REQUIRE(area.any());
  RC_CHECK(area.height() > canvas.height() / 3);
}

RC_TEST("the axis a caller can label is the axis that was drawn") {
  LivePlot plot;
  plot.setMinimumSpan(0.1);
  feed_steady(plot, 200);

  const rc::plot::Range axis = plot.axis();
  RC_CHECK_NEAR(rc::plot::span(axis), 0.1, 1e-9);
  RC_CHECK(axis.low < 12.0);
  RC_CHECK(axis.high > 12.0);
}

RC_TEST("the axis is announced when it changes, so a label can follow it") {
  // The signal is why this class needs Q_OBJECT. Without the macro there is no
  // definition for it and the link fails.
  LivePlot plot;
  plot.setMinimumSpan(0.1);

  int announcements = 0;
  double last_low = 0.0;
  double last_high = 0.0;
  QObject::connect(&plot, &LivePlot::axisChanged, [&](double low, double high) {
    ++announcements;
    last_low = low;
    last_high = high;
  });

  for (int i = 0; i < 50; ++i) plot.addSample(static_cast<double>(i) * 0.05, 12.0);
  render(plot, QSize(200, 120));
  RC_REQUIRE(announcements >= 1);
  RC_CHECK_NEAR(last_high - last_low, 0.1, 1e-9);

  // Drawing the same data again announces nothing new.
  const int before = announcements;
  render(plot, QSize(200, 120));
  RC_CHECK_EQ(announcements, before);

  // A change in the signal moves the axis and is announced.
  for (int i = 50; i < 100; ++i) plot.addSample(static_cast<double>(i) * 0.05, 15.0);
  render(plot, QSize(200, 120));
  RC_CHECK(announcements > before);
}

RC_TEST("old samples leave the widget as time moves on") {
  LivePlot plot(2.0, 4000);   // a two second window
  for (int i = 0; i < 500; ++i) plot.addSample(static_cast<double>(i) * 0.01, 1.0);

  const rc::plot::Series& series = plot.series();
  RC_CHECK(series.newest_time() - series.oldest_time() <= 2.01);
  RC_CHECK(series.size() < 500);
}

RC_TEST("a fast signal cannot grow the widget's memory without limit") {
  LivePlot plot(1e9, 256);   // a window long enough never to drop by age
  for (int i = 0; i < 20000; ++i) plot.addSample(static_cast<double>(i) * 0.0001, 1.0);
  RC_CHECK_EQ(plot.series().size(), static_cast<std::size_t>(256));
}

int main(int argc, char** argv) {
  QApplication application(argc, argv);
  return rc::test::run_all();
}
