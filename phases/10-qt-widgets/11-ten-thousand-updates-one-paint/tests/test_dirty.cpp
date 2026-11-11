// A widget cannot exist before a QApplication does, so the framework's own main is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QRect>
#include <QtWidgets/QApplication>

#include <iostream>

#include "solution.hpp"

namespace {

void settle() {
  for (int i = 0; i < 4; ++i) QCoreApplication::processEvents();
}

// A chart that is shown and has had its first paint.
void show_and_settle(ChartWidget& w) {
  w.show();
  settle();
}

}  // namespace

RC_TEST("a column is a rectangle one pixel wide and as high as the chart") {
  const QRect r = ChartWidget::column_rect(5, 100);
  RC_CHECK_EQ(r.x(), 5);
  RC_CHECK_EQ(r.y(), 0);
  RC_CHECK_EQ(r.width(), 1);
  RC_CHECK_EQ(r.height(), 100);
  RC_CHECK(r.contains(5, 99));
  RC_CHECK(!r.contains(6, 0));
  RC_CHECK(!r.contains(4, 0));
}

RC_TEST("ten thousand updates before the event loop runs are one paint") {
  ChartWidget w;
  show_and_settle(w);
  const int before = w.paints();
  for (int i = 0; i < 10000; ++i) w.add_sample(i % 90);
  RC_CHECK_EQ(w.paints(), before);                 // nothing is painted inside the call
  settle();
  std::cout << "  10000 samples added, " << w.paints() - before << " paint after the loop ran, over " << w.columns_drawn() << " columns\n\n";
  RC_CHECK_EQ(w.paints() - before, 1);
  RC_CHECK(w.columns_drawn() >= 1 && w.columns_drawn() <= ChartWidget::kColumns);
}

RC_TEST("a repaint is a paint every time, and an update is one paint however many there are") {
  ChartWidget w;
  show_and_settle(w);
  const int before = w.paints();
  for (int i = 0; i < 100; ++i) w.repaint();
  const int after_repaints = w.paints() - before;
  const int mid = w.paints();
  for (int i = 0; i < 100; ++i) w.update();
  settle();
  const int after_updates = w.paints() - mid;
  std::cout << "  100 repaint() calls: " << after_repaints << " paints at once; 100 update() calls: " << after_updates << " paint after the loop ran\n\n";
  RC_CHECK_EQ(after_repaints, 100);
  RC_CHECK_EQ(after_updates, 1);
}

RC_TEST("one new sample repaints the one column that changed, and not the chart") {
  ChartWidget w;
  show_and_settle(w);
  w.update();
  settle();
  const int full_columns = w.columns_drawn();
  const int before = w.paints();
  w.add_sample(40);
  settle();
  const QRect rect = w.last_paint_rect();
  std::cout << "  a full update went over " << full_columns << " columns; one sample: " << w.paints() - before << " paint, rectangle x " << rect.x() << " width " << rect.width()
            << " height " << rect.height() << ", over " << w.columns_drawn() << " columns\n\n";
  RC_CHECK_EQ(full_columns, ChartWidget::kColumns);
  RC_CHECK_EQ(w.paints() - before, 1);
  RC_CHECK(rect.contains(ChartWidget::column_rect(0, ChartWidget::kHeight)));     // the first sample is in column 0
  RC_CHECK(rect.width() <= 3);
  RC_CHECK_EQ(rect.height(), ChartWidget::kHeight);
  RC_CHECK(w.columns_drawn() <= 3);
}

RC_TEST("two columns far apart are one paint of the rectangle that holds both") {
  ChartWidget w;
  show_and_settle(w);
  const int before = w.paints();
  w.update(ChartWidget::column_rect(10, ChartWidget::kHeight));
  w.update(ChartWidget::column_rect(200, ChartWidget::kHeight));
  settle();
  const QRect rect = w.last_paint_rect();
  std::cout << "  updates of columns 10 and 200: " << w.paints() - before << " paint, rectangle x " << rect.left() << " to " << rect.right() << ", over " << w.columns_drawn()
            << " columns\n\n";
  RC_CHECK_EQ(w.paints() - before, 1);
  RC_CHECK(rect.left() <= 10);
  RC_CHECK(rect.right() >= 200);
  RC_CHECK(w.columns_drawn() >= 191);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication application(argc, argv);
  return rc::test::run_all();
}
