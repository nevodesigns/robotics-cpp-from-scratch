// A painter needs a QGuiApplication before anything else exists, so the test
// framework's own main is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QPointF>
#include <QtCore/QSize>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// Where the ink is, and how much of it.
struct Ink {
  int count = 0;
  int left = 0, top = 0, right = -1, bottom = -1;
};

Ink ink_of(const QImage& image) {
  Ink ink;
  bool first = true;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QRgb pixel = image.pixel(x, y);
      if (qRed(pixel) >= 200 && qGreen(pixel) >= 200 && qBlue(pixel) >= 200) continue;
      ++ink.count;
      if (first) {
        ink.left = ink.right = x;
        ink.top = ink.bottom = y;
        first = false;
        continue;
      }
      if (x < ink.left) ink.left = x;
      if (y < ink.top) ink.top = y;
      if (x > ink.right) ink.right = x;
      if (y > ink.bottom) ink.bottom = y;
    }
  }
  return ink;
}

// A chart border and a diagonal, drawn in logical coordinates and nothing else.
// The same three lines whatever the ratio is, which is the whole point.
void draw_chart(QPaintDevice* device, QSize logical) {
  QPainter painter(device);
  painter.fillRect(0, 0, logical.width(), logical.height(), Qt::white);
  painter.setPen(hairline(Qt::black));
  painter.drawRect(2, 2, logical.width() - 5, logical.height() - 5);
  painter.drawLine(0, 0, logical.width(), logical.height());
}

constexpr int kLogicalWidth = 100;
constexpr int kLogicalHeight = 60;

}  // namespace

RC_TEST("a surface knows how much memory it needs and how much room it has") {
  const Surface one{QSize(100, 60), 1.0};
  RC_CHECK_EQ(one.device().width(), 100);
  RC_CHECK_EQ(one.device().height(), 60);

  const Surface two{QSize(100, 60), 2.0};
  RC_CHECK_EQ(two.device().width(), 200);
  RC_CHECK_EQ(two.device().height(), 120);

  // Ratios are not always whole numbers. 1.5 and 1.25 are ordinary settings and
  // 101 logical units at 1.5 is 151.5 device pixels, which does not exist.
  const Surface awkward{QSize(101, 61), 1.5};
  RC_CHECK_EQ(awkward.device().width(), 152);
  RC_CHECK_EQ(awkward.device().height(), 92);

  // The logical size is what you paint in, and it does not change.
  RC_CHECK_EQ(awkward.logical.width(), 101);
}

RC_TEST("the canvas carries its ratio, and painting fills it") {
  std::cout << "\n    a " << kLogicalWidth << " by " << kLogicalHeight
            << " chart, painted in logical coordinates\n\n";
  std::cout << "    " << std::left << std::setw(34) << "" << std::right
            << std::setw(12) << "image" << std::setw(10) << "ink" << std::setw(18)
            << "bounding box" << "\n";

  const auto report = [](const char* name, const QImage& image) {
    const Ink ink = ink_of(image);
    std::cout << "    " << std::left << std::setw(34) << name << std::right
              << std::setw(6) << image.width() << "x" << std::setw(5)
              << image.height() << std::setw(10) << ink.count << std::setw(8)
              << ink.left << "," << ink.top << ".." << ink.right << "," << ink.bottom
              << "\n";
    return ink;
  };

  const Surface plain{QSize(kLogicalWidth, kLogicalHeight), 1.0};
  const Surface better{QSize(kLogicalWidth, kLogicalHeight), 2.0};

  QImage at_one = plain.canvas();
  at_one.fill(Qt::white);
  draw_chart(&at_one, plain.logical);
  const Ink one = report("ratio 1", at_one);

  // The bug: an image sized in device pixels that was never told its ratio, so
  // the painter treats its coordinates as device pixels and the drawing lands
  // in the corner.
  QImage untagged(better.device(), QImage::Format_ARGB32);
  untagged.fill(Qt::white);
  draw_chart(&untagged, better.logical);
  const Ink wrong = report("ratio 2, image never told", untagged);

  QImage at_two = better.canvas();
  at_two.fill(Qt::white);
  draw_chart(&at_two, better.logical);
  const Ink right = report("ratio 2, canvas()", at_two);

  std::cout << "\n    the middle row is the same image size as the last and a\n";
  std::cout << "    quarter of the drawing, in the corner. On a machine where\n";
  std::cout << "    the ratio is 1 the two are identical, which is why this\n";
  std::cout << "    reaches the people with better screens\n";

  // Both images are the same size in memory.
  RC_CHECK_EQ(untagged.width(), at_two.width());
  RC_CHECK_EQ(untagged.height(), at_two.height());

  // The untagged one draws in the top left quarter and stops.
  RC_CHECK(wrong.right < better.device().width() * 0.6);
  RC_CHECK(wrong.bottom < better.device().height() * 0.6);

  // The tagged one fills the whole image, to the last row and column.
  RC_CHECK(right.right >= better.device().width() - 2);
  RC_CHECK(right.bottom >= better.device().height() - 2);

  // And it has about twice the ink, because the same lines are twice as long
  // once they cross twice as many pixels. About, rather than exactly: how a
  // rasteriser rounds the ends of a line is its own business and differs
  // between Qt versions, so the claim is the factor rather than the count.
  RC_CHECK(right.count > wrong.count * 1.8);
  RC_CHECK(right.count > one.count * 1.8);

  // At a ratio of one the two ways agree exactly, which is the trap.
  QImage untagged_at_one(plain.device(), QImage::Format_ARGB32);
  untagged_at_one.fill(Qt::white);
  draw_chart(&untagged_at_one, plain.logical);
  RC_CHECK_EQ(ink_of(untagged_at_one).count, one.count);
}

RC_TEST("a pen of width one is not one pixel wide") {
  std::cout << "\n    a horizontal line across a surface at ratio 2\n\n";
  std::cout << "    " << std::left << std::setw(22) << "pen" << std::right
            << std::setw(12) << "ink" << std::setw(16) << "device rows" << "\n";

  const Surface surface{QSize(100, 20), 2.0};
  int rows_at[3] = {0, 0, 0};

  for (int width = 0; width <= 2; ++width) {
    QImage image = surface.canvas();
    image.fill(Qt::white);
    {
      QPainter painter(&image);
      QPen pen(Qt::black);
      pen.setWidth(width);
      painter.setPen(pen);
      painter.drawLine(0, 10, 100, 10);
    }
    const Ink ink = ink_of(image);
    rows_at[width] = ink.bottom - ink.top + 1;
    std::cout << "    " << std::left << std::setw(22)
              << ("setWidth(" + std::to_string(width) + ")") << std::right
              << std::setw(12) << ink.count << std::setw(16) << rows_at[width] << "\n";
  }

  std::cout << "\n    width 0 is not no line, it is the thinnest the device can\n";
  std::cout << "    draw. Width 1 means one logical unit, which is two device\n";
  std::cout << "    pixels here, so a grid drawn that way is twice as heavy on\n";
  std::cout << "    a better screen\n";

  RC_CHECK_EQ(rows_at[0], 1);
  RC_CHECK_EQ(rows_at[1], 2);
  RC_CHECK_EQ(rows_at[2], 4);

  // Which is what hairline() is for: one device pixel at any ratio.
  for (const double ratio : {1.0, 2.0, 3.0}) {
    const Surface any{QSize(60, 20), ratio};
    QImage image = any.canvas();
    image.fill(Qt::white);
    {
      QPainter painter(&image);
      painter.setPen(hairline(Qt::black));
      painter.drawLine(0, 10, 60, 10);
    }
    const Ink ink = ink_of(image);
    RC_CHECK_EQ(ink.bottom - ink.top + 1, 1);
  }
}

RC_TEST("reading a pixel back, with and without the ratio") {
  const Surface surface{QSize(kLogicalWidth, kLogicalHeight), 2.0};

  QImage image = surface.canvas();
  image.fill(Qt::white);
  {
    QPainter painter(&image);
    painter.setPen(hairline(Qt::black));
    // A vertical line at three quarters of the way across, in logical units.
    painter.drawLine(75, 0, 75, kLogicalHeight);
  }

  // Find the column of ink in the image's own pixels.
  const Ink ink = ink_of(image);
  RC_CHECK_EQ(ink.left, ink.right);
  const int device_column = ink.left;

  std::cout << "\n    a line drawn at logical x = 75, on a surface at ratio 2\n\n";
  std::cout << "    " << std::left << std::setw(38) << "found in the image at column"
            << std::right << device_column << "\n";
  std::cout << "    " << std::left << std::setw(38) << "read back with the ratio"
            << std::right << surface.from_device(QPointF(device_column, 0)).x() << "\n";
  std::cout << "    " << std::left << std::setw(38) << "read back without it"
            << std::right << device_column << "\n";

  // The line is at device column 150, and dividing by the ratio puts it back
  // where it was drawn.
  RC_CHECK_EQ(device_column, 150);
  RC_CHECK_NEAR(surface.from_device(QPointF(device_column, 0)).x(), 75.0, 0.5);

  // Taken as a logical coordinate directly it is out by exactly the ratio, and
  // it is off the chart entirely: 150 in a chart 100 wide.
  RC_CHECK(device_column > kLogicalWidth);

  // The two conversions undo each other.
  const QPointF there = surface.to_device(QPointF(12.5, 33.0));
  RC_CHECK_NEAR(there.x(), 25.0, 1e-12);
  const QPointF back = surface.from_device(there);
  RC_CHECK_NEAR(back.x(), 12.5, 1e-12);
  RC_CHECK_NEAR(back.y(), 33.0, 1e-12);

  // A ratio of zero is refused rather than dividing by it.
  const Surface broken{QSize(10, 10), 0.0};
  RC_CHECK_NEAR(broken.from_device(QPointF(4.0, 5.0)).x(), 4.0, 1e-12);
}

int main(int argc, char** argv) {
  // Offscreen, so this runs on a machine with no display at all.
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
