#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QPointF>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QWidget>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>
#include <rc/plot/decimate.hpp>
#include <rc/plot/palette.hpp>
#include <rc/plot/pick.hpp>
#include <rc/plot/series.hpp>
#include <rc/plot/text_chart.hpp>
#include <rc/plot/ticks.hpp>

// Phase 10 ends with a live chart of any signal, in a window and in a terminal, that does not lie about a steady one. Every piece is a lesson of this phase:
//
//   the columns of lowest and highest value      10-06: a million samples are at most two points a pixel, and a spike is still there
//   the range, with room and a minimum span      10-07: a steady signal is a flat line, and noise in its last digit is not a storm
//   the ticks on nice numbers                    10-05
//   the map from data to pixels                  10-08
//   a series colour that shows on white          10-13
//   a repaint that is asked for and not forced   10-11
//
// The point of one model is that the two charts cannot disagree. The range is worked out once, by the model, from the samples in the window; the
// text chart and the window both draw from it, so a signal that is a flat line in one is a flat line in the other, and a spike is one tall column in both.

using rc::plot::Column;
using rc::plot::ChartView;
using rc::plot::decimate;
using rc::plot::Range;
using rc::plot::Rgb;
using rc::plot::Sample;
using rc::plot::series_color;
using rc::plot::Ticks;
using rc::plot::ticks_for;
using rc::plot::to_pixel;
using rc::plot::value_range;

// The samples of one signal, the part of time to show, and the smallest span of values that counts as a signal and not as noise.
class ChartModel {
 public:
  ChartModel() = default;

  void add(double time, double value) {
    Sample s;
    s.time = time;
    s.value = value;
    samples_.push_back(s);
  }
  std::size_t size() const { return samples_.size(); }
  void set_window(double t0, double t1) {
    t0_ = t0;
    t1_ = t1;
  }
  void set_min_span(double span) { min_span_ = span; }
  double t0() const { return t0_; }
  double t1() const { return t1_; }
  const std::vector<Sample>& samples() const { return samples_; }

  // The range of values to show, from the samples inside the window: the lowest and highest with room around them, never narrower than the minimum span.
  // One column holding the whole window is the lowest and highest of every sample in it.
  Range range() const {
    // TODO: value_range(decimate(samples_, t0_, t1_, 1), min_span_).
    return Range{0.0, 1.0};
  }

  // The map from data to a chart `across` by `down` pixels, with a margin.
  ChartView view(double across, double down, double margin) const {
    // TODO: a ChartView with t0, t1, range(), across, down and margin filled in.
    (void)across;
    (void)down;
    (void)margin;
    return ChartView();
  }

  // The same chart as text, from the same samples, window and minimum span.
  std::vector<std::string> text(int columns, int rows) const {
    // TODO: rc::plot::render_text(samples_, t0_, t1_, columns, rows, min_span_).
    (void)columns;
    (void)rows;
    return std::vector<std::string>();
  }

 private:
  std::vector<Sample> samples_;
  double t0_ = 0.0;
  double t1_ = 1.0;
  double min_span_ = 0.0;
};

// The chart as a widget: a column of ink for each pixel of width, from the lowest value in it to the highest, with a short grey mark on the left at each tick.
class SignalChart : public QWidget {
 public:
  static constexpr int kTickTarget = 5;

  explicit SignalChart(QWidget* parent = nullptr) : QWidget(parent) {}

  ChartModel& model() { return model_; }
  const ChartModel& model() const { return model_; }

  // Adds a sample and asks for a repaint, which Qt does once when the loop is free.
  void add_sample(double time, double value) {
    model_.add(time, value);
    update();
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    // TODO: v = model_.view(width(), height(), 0). Ticks: ticks = ticks_for(v.range.low, v.range.high, kTickTarget); with a grey pen (QColor(150, 150, 150)),
    // for each tick k draw a line from (0, y) to (3, y), with y = lround(to_pixel(v, v.t0, ticks.value(k)).y).
    // Ink: c = series_color(0); pen QColor(c.r, c.g, c.b). columns = decimate(model_.samples(), v.t0, v.t1, width()). For each x with columns[x].n > 0,
    // top = lround(to_pixel(v, v.t0, high).y) and bottom = the same for low; draw a vertical line from (x, top) to (x, bottom).
  }

 private:
  ChartModel model_;
};

#endif  // LESSON_SOLUTION_HPP
