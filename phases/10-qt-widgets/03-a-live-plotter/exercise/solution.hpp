#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QWidget>

#include <rc/plot/series.hpp>

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
    // TODO: keep the sample, and record that the picture is out of date.
    //
    // Do not ask for a repaint here. A sensor at a kilohertz would ask for a
    // thousand a second and a screen shows sixty, so the widget would spend its
    // time preparing pictures nobody will ever see, and the interface stops
    // responding while it does.
    (void)time;
    (void)value;
  }

  // Called at the frame rate, by a timer or by whatever owns the widget. This
  // is the only place a repaint is ever asked for.
  void refresh() {
    // TODO: ask for a repaint, but only if something changed since the last one.
    //
    // A frame timer runs whether or not data arrived, and a chart that
    // repaints on every tick regardless burns a core drawing the same picture.
    //
    // Count the requests in repaint_requests_, which is what the tests watch.
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
    // TODO: the range of the samples, with room around it, and a floor under
    // its span so a steady signal is not drawn as noise. Lesson 10-02 has all
    // three pieces.
    return rc::plot::Range{0.0, 1.0};
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

    // TODO: announce the axis when it changes, and only when it changes, so a
    // label beside the chart can follow it without polling and without being
    // told the same thing sixty times a second.

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

#endif  // LESSON_SOLUTION_HPP
