// A painter needs a QGuiApplication before anything else exists, so the framework's own main is suppressed and one is written below.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QPointF>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include <iostream>

#include "solution.hpp"

namespace {

// A small chart: a frame, a horizontal line and a diagonal that is antialiased, drawn at an offset so that two renderings can differ by a fraction of a pixel.
QImage chart(double offset, QImage::Format format = QImage::Format_ARGB32) {
  QImage image(80, 60, format);
  image.fill(Qt::white);
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(QPen(Qt::black, 1.0));
  painter.drawRect(QRectF(2.5, 2.5, 75.0, 55.0));
  painter.drawLine(QPointF(5.0 + offset, 30.0), QPointF(75.0 + offset, 30.0));
  painter.drawLine(QPointF(5.0 + offset, 8.0 + offset), QPointF(75.0 + offset, 52.0 + offset));
  return image;
}

// The same picture with n pixels changed to red.
QImage with_stray_pixels(QImage image, int n) {
  for (int i = 0; i < n; ++i) image.setPixel(10 + 3 * i, 20, qRgb(255, 0, 0));
  return image;
}

}  // namespace

RC_TEST("the same chart drawn a tenth of a pixel apart is different to an exact comparison, and the same to a person") {
  const QImage a = chart(0.0);
  const QImage b = chart(0.1);
  const Diff exact = compare(a, b, 0);
  const Diff loose = compare(a, b, 64);
  std::cout << "  a tenth of a pixel apart: " << exact.differing << " pixels differ exactly, the largest difference is " << exact.max_delta << " of 255, and " << loose.differing
            << " differ by more than 64\n\n";
  RC_CHECK(exact.differing > 0);
  RC_CHECK(exact.max_delta > 0);
  RC_CHECK(exact.max_delta <= 64);
  RC_CHECK_EQ(loose.differing, 0);
  RC_CHECK(images_match(a, b, 64, 0));
  RC_CHECK(!images_match(a, b, 0, 0));
  RC_CHECK(images_match(a, a, 0, 0));
}

RC_TEST("one wrong pixel is found at any tolerance below 255, with the rectangle that holds it") {
  const QImage a = chart(0.0);
  QImage b = a;
  b.setPixel(37, 12, qRgb(255, 0, 0));
  const Diff d = compare(a, b, 10);
  const Diff strict = compare(a, b, 254);
  const Diff blind = compare(a, b, 255);
  std::cout << "  one red pixel at (37, 12): " << d.differing << " differs, bounds " << d.bounds.x() << ", " << d.bounds.y() << " size " << d.bounds.width() << " by " << d.bounds.height()
            << ", largest difference " << d.max_delta << "; at a tolerance of 254 " << strict.differing << ", at 255 " << blind.differing << "\n\n";
  RC_CHECK_EQ(d.differing, 1);
  RC_CHECK_EQ(d.bounds.x(), 37);
  RC_CHECK_EQ(d.bounds.y(), 12);
  RC_CHECK_EQ(d.bounds.width(), 1);
  RC_CHECK_EQ(d.bounds.height(), 1);
  RC_CHECK_EQ(strict.differing, 1);
  RC_CHECK_EQ(blind.differing, 0);           // no channel can be more than 255 apart: a tolerance of 255 sees nothing
  RC_CHECK(d.max_delta >= 200);

  QImage c = a;
  c.setPixel(4, 40, qRgb(0, 0, 255));
  c.setPixel(70, 50, qRgb(0, 0, 255));
  const Diff two = compare(a, c, 10);
  RC_CHECK_EQ(two.differing, 2);
  RC_CHECK_EQ(two.bounds.x(), 4);
  RC_CHECK_EQ(two.bounds.right(), 70);       // the rectangle holds both
  RC_CHECK_EQ(two.bounds.y(), 40);
  RC_CHECK_EQ(two.bounds.bottom(), 50);
  RC_CHECK(compare(a, a, 0).bounds.isNull());
}

RC_TEST("images of different sizes never match, whatever their pixels are") {
  QImage a(100, 50, QImage::Format_ARGB32);
  QImage b(100, 51, QImage::Format_ARGB32);
  a.fill(Qt::white);
  b.fill(Qt::white);
  const Diff d = compare(a, b, 0);
  std::cout << "  100 by 50 against 100 by 51, both white: size mismatch " << d.size_mismatch << ", matches at a tolerance of 255 and any count: " << images_match(a, b, 255, 100000)
            << "\n\n";
  RC_CHECK(d.size_mismatch);
  RC_CHECK(!images_match(a, b, 255, 100000));
  RC_CHECK(!images_match(a, QImage(), 255, 100000));
}

RC_TEST("the same picture in different formats is the same picture") {
  const QImage a = chart(0.0, QImage::Format_ARGB32);
  const QImage b = chart(0.0, QImage::Format_RGB32);
  const QImage c = chart(0.0, QImage::Format_ARGB32_Premultiplied);
  const QImage d = chart(0.0, QImage::Format_RGBA8888);
  const Diff ab = compare(a, b, 0);
  const Diff ac = compare(a, c, 0);
  const Diff ad = compare(a, d, 0);
  std::cout << "  ARGB32 against RGB32: " << ab.differing << " differ; against premultiplied: " << ac.differing << "; against RGBA8888: " << ad.differing << "\n\n";
  RC_CHECK_EQ(ab.differing, 0);
  RC_CHECK_EQ(ac.differing, 0);
  RC_CHECK_EQ(ad.differing, 0);
  RC_CHECK(compare(a, chart(0.0), 0).differing == 0);
}

RC_TEST("a few stray pixels pass a count that allows them, and a chart moved by a pixel does not") {
  const QImage a = chart(0.0);
  const QImage three = with_stray_pixels(a, 3);
  const Diff d3 = compare(a, three, 10);
  const QImage moved = chart(1.0);
  const Diff dm = compare(a, moved, 10);
  std::cout << "  three stray pixels: " << d3.differing << " differ; the chart moved by one pixel: " << dm.differing << " differ\n\n";
  RC_CHECK_EQ(d3.differing, 3);
  RC_CHECK(images_match(a, three, 10, 5));
  RC_CHECK(!images_match(a, three, 10, 2));
  RC_CHECK(dm.differing > 100);
  RC_CHECK(!images_match(a, moved, 10, 5));
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication application(argc, argv);
  return rc::test::run_all();
}
