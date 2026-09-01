// rc/qt/live_plot.hpp
//
// The live chart from lesson 10-03, graduated.
//
// The arithmetic is entirely rc::plot, unchanged from the terminal version in
// lesson 10-02. What this adds is the one thing a terminal never had to
// answer: how often to draw.
//
// A sensor produces data at whatever rate the hardware runs, a screen shows
// about sixty frames a second, and tying the two together lets the faster one
// set the pace for everything. So adding a sample never repaints; it records
// that the picture is out of date, and refresh, driven at the frame rate, asks
// for the repaint and only when something changed. A thousand samples cost one
// repaint, and a frame with nothing new costs nothing.
//
// If data arrives on another thread, this is not enough on its own: a widget
// may only be touched from the thread that owns it, and the crossing is what a
// queued signal is for.

#ifndef RC_QT_LIVE_PLOT_HPP
#define RC_QT_LIVE_PLOT_HPP

#include <cstddef>

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QWidget>

#include <rc/plot/series.hpp>

namespace rc {
namespace qt {

// A chart that is written to at the rate a sensor produces data and read at the
// rate a screen can show it, which are not the same rate and must not be tied
// together.
//
// A sensor at one kilohertz asking for a repaint every sample asks for a
// thousand a second. A screen shows sixty. The widget spends its time
// preparing pictures nobody will ever see, and the interface stops responding
// while it does, which reads as the program hanging rather than as a drawing
// problem.
//
// So adding a sample never repaints. It records that the picture is out of
// date, and something running at the frame rate asks for the repaint.
class LivePlot : public QWidget {
  Q_OBJECT

 public:
  explicit LivePlot(double window = 10.0, std::size_t capacity = 4000,
                    QWidget* parent = nullptr)
      : QWidget(parent), series_(capacity, window) {}

  // Called at the data rate, which may be thousands of times a second.
  void addSample(double time, double value) {
    series_.add(time, value);
    stale_ = true;
  }

  // Called at the frame rate, by a timer or by whatever owns the widget. This
  // is the only place a repaint is ever asked for.
  void refresh() {
    if (!stale_) return;
    stale_ = false;
    ++repaint_requests_;
    update();
  }

  void setMinimumSpan(double span) { minimum_span_ = span; }
  double minimumSpan() const { return minimum_span_; }

  bool stale() const { return stale_; }
  int repaintRequests() const { return repaint_requests_; }
  int paintCount() const { return paint_count_; }
  const rc::plot::Series& series() const { return series_; }

  // The axis actually drawn, which is the fitted range with room around it and
  // a floor under its span. Exposed because a caller wanting to label the axis
  // needs the same numbers the painting used.
  rc::plot::Range axis() const {
    return rc::plot::at_least(rc::plot::padded(rc::plot::range_of(series_), 0.1),
                              minimum_span_);
  }

 signals:
  // Emitted when the drawn axis changes, so a label beside the chart can follow
  // it without polling. This is why the class needs Q_OBJECT: without the macro
  // the signal has no definition and the link fails.
  void axisChanged(double low, double high);

 protected:
  void paintEvent(QPaintEvent*) override {
    ++paint_count_;

    QPainter painter(this);
    painter.fillRect(rect(), background_);
    if (series_.size() < 2) return;

    const rc::plot::Range range = axis();
    if (range.low != last_low_ || range.high != last_high_) {
      last_low_ = range.low;
      last_high_ = range.high;
      emit axisChanged(range.low, range.high);
    }

    const double across = static_cast<double>(width());
    const double down = static_cast<double>(height());

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(trace_, 2.0));

    for (std::size_t i = 1; i < series_.size(); ++i) {
      const rc::plot::Point from =
          rc::plot::place_sample(series_.at(i - 1), series_.newest_time(),
                                 series_.window(), range, across, down, margin_);
      const rc::plot::Point to =
          rc::plot::place_sample(series_.at(i), series_.newest_time(),
                                 series_.window(), range, across, down, margin_);
      painter.drawLine(QPointF(from.across, from.down), QPointF(to.across, to.down));
    }
  }

 private:
  rc::plot::Series series_;
  double minimum_span_ = 0.1;
  double margin_ = 8.0;
  bool stale_ = false;
  int repaint_requests_ = 0;
  int paint_count_ = 0;
  double last_low_ = 0.0;
  double last_high_ = 0.0;

  QColor background_{20, 24, 22};
  QColor trace_{79, 189, 179};
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_LIVE_PLOT_HPP
