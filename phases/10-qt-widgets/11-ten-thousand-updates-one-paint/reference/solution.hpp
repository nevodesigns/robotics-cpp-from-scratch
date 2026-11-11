#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>
#include <vector>

// A chart is redrawn when it is told to be. Two calls say so, and they are not the same. update() says the picture is out of date and schedules ONE paint
// for when the event loop is next free, however many times it is called. repaint() paints now, every time. A sensor that adds a sample a millisecond and calls
// repaint() is a sensor that paints a thousand times a second.
//
// And a paint can be told how much of the chart is out of date: update(rect) marks a rectangle, the paint event carries it, and a paintEvent that loops
// over only the columns in that rectangle does a hundredth of the work when one column has changed.

// A strip chart 300 columns wide and 100 high. Each sample is a height, drawn as a vertical line in the next column, wrapping at the right edge.
class ChartWidget : public QWidget {
 public:
  static constexpr int kColumns = 300;
  static constexpr int kHeight = 100;

  explicit ChartWidget(QWidget* parent = nullptr) : QWidget(parent), levels_(kColumns, -1) { setFixedSize(kColumns, kHeight); }

  // The rectangle that column `column` is drawn in: one pixel wide and the whole height of the chart.
  static QRect column_rect(int column, int height) { return QRect(column, 0, 1, height); }

  // Records a sample in the next column and asks for a repaint of that column only.
  void add_sample(int level) {
    const int column = next_ % kColumns;
    levels_[static_cast<std::size_t>(column)] = level;
    ++next_;
    update(column_rect(column, kHeight));
  }

  int paints() const { return paints_; }                  // paint events so far
  QRect last_paint_rect() const { return last_rect_; }    // the rectangle the last one was for
  int columns_drawn() const { return drawn_; }            // columns that the last one went over

 protected:
  void paintEvent(QPaintEvent* event) override {
    ++paints_;
    last_rect_ = event->rect();
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::white);
    painter.setPen(Qt::black);
    drawn_ = 0;
    const int first = event->rect().left() > 0 ? event->rect().left() : 0;
    const int last = event->rect().right() < kColumns - 1 ? event->rect().right() : kColumns - 1;
    for (int c = first; c <= last; ++c) {
      ++drawn_;
      const int level = levels_[static_cast<std::size_t>(c)];
      if (level >= 0) painter.drawLine(c, kHeight - 1, c, kHeight - 1 - level);
    }
  }

 private:
  std::vector<int> levels_;
  int next_ = 0;
  int paints_ = 0;
  int drawn_ = 0;
  QRect last_rect_;
};

#endif  // LESSON_SOLUTION_HPP
