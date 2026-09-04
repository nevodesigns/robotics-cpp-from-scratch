// rc/qt/surface.hpp
//
// The two kinds of pixel, kept apart, from lesson 10-04.
//
// A window 100 units wide on a screen that draws two device pixels for each of
// them is 200 pixels of memory and 100 of arithmetic. You paint in the second
// kind and the image is made of the first, and every high resolution display
// bug is one of those two being used where the other belongs.
//
// Measured on a 100 by 60 chart at a ratio of 2:
//
//   image never told its ratio    200x120     397 ink   0,0..100,60
//   canvas()                      200x120     797 ink   0,0..199,119
//
// The first is the same image size as the second and a quarter of the drawing,
// in the corner. At a ratio of 1 the two are identical, which is why this
// reaches the people with better screens rather than the person who wrote it.

#ifndef RC_QT_SURFACE
#define RC_QT_SURFACE

#include <QtCore/QPointF>
#include <QtCore/QSize>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPen>

#include <cmath>

namespace rc {
namespace qt {

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

  // How many pixels of memory this needs.
  //
  // Rounded, because a ratio is not always a whole number: 1.5 and 1.25 are
  // ordinary settings, and 101 logical units at 1.5 is 151.5 device pixels,
  // which does not exist.
  QSize device() const {
    return QSize(static_cast<int>(std::lround(logical.width() * ratio)),
                 static_cast<int>(std::lround(logical.height() * ratio)));
  }

  // An image to paint into.
  //
  // Sized in device pixels and told what its ratio is, so that a painter on it
  // accepts logical coordinates and fills the whole thing. Forgetting the
  // second half is the bug above: the image is the right size and everything
  // drawn on it is a fraction of the size it should be.
  QImage canvas() const {
    QImage image(device(), QImage::Format_ARGB32);
    image.setDevicePixelRatio(ratio);
    return image;
  }

  // A position in the image's own pixels, back into the coordinates it was
  // painted in.
  //
  // Anything that reads a rendered image and asks what it is looking at needs
  // this: a test asserting where a line landed, a hit test against a chart, a
  // screenshot compared against an expectation. Done without the ratio it is
  // out by exactly the ratio, which on a 1.0 machine is not out at all, which
  // is why it survives review.
  QPointF from_device(QPointF pixel) const {
    if (ratio == 0.0) return pixel;
    return QPointF(pixel.x() / ratio, pixel.y() / ratio);
  }

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

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_SURFACE
