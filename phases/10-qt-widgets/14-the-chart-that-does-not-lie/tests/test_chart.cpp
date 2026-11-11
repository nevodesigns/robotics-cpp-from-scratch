// A widget cannot exist before a QApplication does, so the framework's own main is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kWidth = 200;
constexpr int kHeight = 130;
constexpr int kRows = 13;
constexpr int kTextColumns = 50;

// The window drawn into an image.
QImage picture(SignalChart& chart) {
  chart.resize(kWidth, kHeight);
  QImage image(kWidth, kHeight, QImage::Format_ARGB32);
  image.fill(Qt::white);
  chart.render(&image);
  return image;
}

bool is_ink(QRgb p) { return qRed(p) < 235 || qGreen(p) < 235 || qBlue(p) < 235; }

// Rows of the picture that have any ink to the right of the tick marks.
int ink_rows(const QImage& image) {
  int rows = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 5; x < image.width(); ++x) {
      if (is_ink(image.pixel(x, y))) {
        ++rows;
        break;
      }
    }
  }
  return rows;
}

int first_ink_row(const QImage& image) {
  for (int y = 0; y < image.height(); ++y)
    for (int x = 5; x < image.width(); ++x)
      if (is_ink(image.pixel(x, y))) return y;
  return -1;
}

int last_ink_row(const QImage& image) {
  for (int y = image.height() - 1; y >= 0; --y)
    for (int x = 5; x < image.width(); ++x)
      if (is_ink(image.pixel(x, y))) return y;
  return -1;
}

int text_rows_marked(const std::vector<std::string>& lines) {
  int n = 0;
  for (const std::string& line : lines) {
    const std::size_t bar = line.find('|');
    if (bar != std::string::npos && line.find('*', bar) != std::string::npos) ++n;
  }
  return n;
}

void fill_steady(SignalChart& chart) {
  chart.model().set_window(0.0, 399.0);
  for (int i = 0; i < 400; ++i) chart.model().add(static_cast<double>(i), 5.0 + ((i % 2) ? 1e-6 : -1e-6));
}

}  // namespace

RC_TEST("a steady signal with noise in its last digit is a flat line in the terminal and in the window") {
  SignalChart chart;
  fill_steady(chart);
  chart.model().set_min_span(1.0);
  const std::vector<std::string> text = chart.model().text(kTextColumns, kRows);
  const QImage image = picture(chart);
  const int top = first_ink_row(image);
  const int bottom = last_ink_row(image);
  std::cout << "  5 plus or minus 1e-6, a minimum span of 1: the text marks " << text_rows_marked(text) << " of " << kRows << " rows; the window has ink on " << ink_rows(image)
            << " of " << kHeight << " pixel rows, from " << top << " to " << bottom << "\n";
  RC_CHECK_EQ(static_cast<int>(text.size()), kRows);
  RC_CHECK_EQ(text_rows_marked(text), 1);
  RC_CHECK(ink_rows(image) >= 1 && ink_rows(image) <= 3);
  RC_CHECK(top >= 0);
  RC_CHECK(std::abs((top + bottom) / 2 - kHeight / 2) <= 3);          // in the middle of the window, as the middle row is of the text
  const Range r = chart.model().range();
  RC_CHECK_NEAR(r.high - r.low, 1.0, 1e-9);
  RC_CHECK_NEAR((r.high + r.low) / 2.0, 5.0, 1e-6);
}

RC_TEST("without a minimum span both charts show the noise as a storm, together") {
  SignalChart chart;
  fill_steady(chart);
  chart.model().set_min_span(0.0);
  const std::vector<std::string> text = chart.model().text(kTextColumns, kRows);
  const QImage image = picture(chart);
  std::cout << "  the same signal with no minimum span: the text marks " << text_rows_marked(text) << " of " << kRows << " rows, the window has ink on " << ink_rows(image)
            << " of " << kHeight << " pixel rows\n\n";
  RC_CHECK(text_rows_marked(text) >= 10);
  RC_CHECK(ink_rows(image) >= 100);
  RC_CHECK(chart.model().range().high - chart.model().range().low < 1e-4);
}

RC_TEST("a spike is one tall column in the terminal and in the window, in the same place") {
  SignalChart chart;
  chart.model().set_window(0.0, 9999.0);
  chart.model().set_min_span(0.1);
  for (int i = 0; i < 10000; ++i) chart.model().add(static_cast<double>(i), i == 7000 ? 5.0 : 0.001 * std::sin(i * 0.1));
  const std::vector<std::string> text = chart.model().text(kTextColumns, 15);
  RC_CHECK_EQ(static_cast<int>(text.size()), 15);

  // The column of the text where the highest mark is.
  int text_column = -1;
  for (const std::string& line : text) {
    const std::size_t bar = line.find('|');
    const std::size_t star = line.find('*', bar);
    if (star != std::string::npos) {
      text_column = static_cast<int>(star - bar - 1);
      break;
    }
  }
  const QImage image = picture(chart);
  const int top = first_ink_row(image);
  int image_column = -1;
  for (int x = 5; x < kWidth && top >= 0; ++x)
    if (is_ink(image.pixel(x, top))) {
      image_column = x;
      break;
    }
  // How tall the column is in the picture.
  int tall = 0;
  for (int y = 0; y < kHeight && image_column >= 0; ++y)
    if (is_ink(image.pixel(image_column, y))) ++tall;
  std::cout << "  a spike of 5 at sample 7000 of 10000: the text's highest mark is in column " << text_column << " of " << kTextColumns << ", the window's top ink is in column "
            << image_column << " of " << kWidth << " and the column is " << tall << " pixels tall\n\n";
  RC_CHECK(text_column >= 0);
  RC_CHECK(image_column >= 0);
  const double text_fraction = (text_column + 0.5) / kTextColumns;
  const double image_fraction = (image_column + 0.5) / kWidth;
  RC_CHECK(std::fabs(text_fraction - image_fraction) <= 1.0 / kTextColumns);      // the same place, to the width of a text column
  RC_CHECK(tall >= kHeight / 2);
}

RC_TEST("the window marks the model's ticks, and the labels of the text are ticks of the model's range") {
  SignalChart chart;
  chart.model().set_window(0.0, 199.0);
  for (int i = 0; i < 200; ++i) chart.model().add(static_cast<double>(i), 10.0 + 10.0 * std::sin(i * 0.05));
  const QImage image = picture(chart);
  const ChartView v = chart.model().view(kWidth, kHeight, 0.0);
  const Ticks ticks = ticks_for(v.range.low, v.range.high, SignalChart::kTickTarget);
  int marked = 0;
  for (int k = 0; k < ticks.count; ++k) {
    const int y = static_cast<int>(std::lround(to_pixel(v, v.t0, ticks.value(k)).y));
    if (y >= 0 && y < kHeight && is_ink(image.pixel(1, y))) ++marked;
  }
  std::cout << "  the model's range is " << v.range.low << " to " << v.range.high << ", " << ticks.count << " ticks, " << marked << " marked in the window\n";
  RC_CHECK(ticks.count >= 3);
  RC_CHECK_EQ(marked, ticks.count);

  const std::vector<std::string> text = chart.model().text(kTextColumns, kRows);
  const Ticks text_ticks = ticks_for(v.range.low, v.range.high, kRows / 3);
  int labels = 0;
  int on_a_tick = 0;
  for (const std::string& line : text) {
    const std::size_t bar = line.find('|');
    if (bar == std::string::npos) continue;
    std::string label = line.substr(0, bar);
    label.erase(std::remove(label.begin(), label.end(), ' '), label.end());
    if (label.empty()) continue;
    ++labels;
    const double value = std::atof(label.c_str());
    for (int k = 0; k < text_ticks.count; ++k)
      if (std::fabs(text_ticks.value(k) - value) < 1e-9) ++on_a_tick;
  }
  std::cout << "  the text has " << labels << " labels, " << on_a_tick << " on a tick of the same range\n\n";
  RC_CHECK(labels >= 2);
  RC_CHECK_EQ(on_a_tick, labels);
}

RC_TEST("an empty chart draws no ink, and a window of no width does not divide by zero") {
  SignalChart chart;
  const QImage empty = picture(chart);
  RC_CHECK_EQ(ink_rows(empty), 0);
  const std::vector<std::string> blank = chart.model().text(kTextColumns, kRows);
  RC_CHECK_EQ(static_cast<int>(blank.size()), kRows);
  RC_CHECK_EQ(text_rows_marked(blank), 0);

  chart.model().add(5.0, 2.0);
  chart.model().set_window(5.0, 5.0);          // no width in time
  const Range r = chart.model().range();
  RC_CHECK(std::isfinite(r.low) && std::isfinite(r.high));
  const QImage one = picture(chart);
  (void)one;
  RC_CHECK_EQ(static_cast<int>(chart.model().text(kTextColumns, kRows).size()), kRows);

  // And a sample added to the widget is in the model, with a repaint asked for.
  SignalChart live;
  live.add_sample(0.0, 1.0);
  live.add_sample(1.0, 2.0);
  RC_CHECK_EQ(static_cast<int>(live.model().size()), 2);
  RC_CHECK(chart.model().size() == 1);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication application(argc, argv);
  return rc::test::run_all();
}
