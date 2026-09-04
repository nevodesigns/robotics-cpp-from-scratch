#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QPointF>
#include <QtCore/QSize>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPen>

#include <cmath>

// The two kinds of pixel, kept apart.
//
// A window 100 units wide on a screen that draws two device pixels for each of
// them is 200 pixels of memory and 100 of arithmetic. You paint in the second
// kind and the image is made of the first, and every high resolution display
// bug is one of those two being used where the other belongs.
//
// Measured on a 100 by 60 drawing at a ratio of 2, with the ratio never set on
// the image: 397 pixels of ink inside a bounding box of 0,0 to 100,60, in an
// image 200 by 120. The chart is drawn at quarter size in the corner. With the
// ratio set: 1621 pixels of ink, filling 0,0 to 199,119.
struct Surface {
  QSize logical;
  double ratio = 1.0;

  // TODO 1: how many pixels of memory this needs.
  //
  // The logical size times the ratio, rounded to whole pixels with std::lround.
  //
  // Rounded, because a ratio is not always a whole number: 1.5 and 1.25 are
  // ordinary settings, and 101 logical units at 1.5 is 151.5 device pixels,
  // which does not exist.
  QSize device() const { return logical; }

  // TODO 2: an image to paint into.
  //
  // Sized in device pixels, from device(), and then told what its ratio is with
  // setDevicePixelRatio. Both halves matter and the second is the one people
  // leave out.
  //
  // With it, a painter on the image accepts logical coordinates and the drawing
  // fills the whole thing. Without it, the image is the right size and
  // everything drawn on it lands in the top left corner at a fraction of the
  // size: measured, 397 pixels of ink in a box 100 by 60, inside an image 200
  // by 120 that should have held 797 filling all of it.
  QImage canvas() const {
    return QImage(logical, QImage::Format_ARGB32);
  }

  // TODO 3: a position in the image's own pixels, back into the coordinates it
  // was painted in.
  //
  // Divide by the ratio, and return the point unchanged if the ratio is zero
  // rather than dividing by it.
  //
  // Anything that reads a rendered image and asks what it is looking at needs
  // this: a test asserting where a line landed, a hit test against a chart, a
  // screenshot compared against an expectation. Done without the ratio it is
  // out by exactly the ratio, which on a machine where the ratio is 1 is not
  // out at all, which is why it survives review.
  QPointF from_device(QPointF pixel) const { return pixel; }

  QPointF to_device(QPointF logical_point) const {
    return QPointF(logical_point.x() * ratio, logical_point.y() * ratio);
  }
};

// A pen that is one device pixel wide whatever the ratio.
//
// Qt calls a pen of width zero cosmetic, and it means the thinnest line the
// device can draw rather than no line at all. Measured at a ratio of 2 over a
// hundred logical units: width 0 puts ink in one device row, width 1 in two,
// width 2 in four.
//
// So a grid drawn with setWidth(1) is twice as heavy on a better screen, which
// is usually not what anybody meant by "one pixel".
inline QPen hairline(const QColor& colour) {
  QPen pen(colour);
  pen.setWidth(0);
  return pen;
}

#endif  // LESSON_SOLUTION_HPP
