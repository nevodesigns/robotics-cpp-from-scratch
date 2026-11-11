#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QRect>
#include <QtGui/QImage>
#include <QtGui/QRgb>
#include <cstdlib>

// A test of a chart can render it to an image and compare it with one that is known to be right. An exact comparison is the first idea, and it fails the day a
// platform antialiases a line a tenth of a pixel differently: the pictures look the same to a person and are not the same to the computer. A comparison that
// ignores everything fails the other way, and misses the one pixel that is wrong.
//
// The middle is two numbers: how far apart a pixel's channels may be before it counts as different (the tolerance), and how many pixels may differ before the
// pictures are (the count). And a result that says where, so that a failure can be looked at.

struct Diff {
  int differing = 0;            // pixels with a channel more than `tolerance` apart
  int max_delta = 0;            // the largest difference in any channel of any pixel
  QRect bounds;                 // the smallest rectangle holding every differing pixel, null if there are none
  bool size_mismatch = false;   // the images are not the same size: nothing else is meaningful
};

// Compares two images pixel by pixel, through QImage::pixel so that their formats need not match. Different sizes are a mismatch and nothing is counted.
inline Diff compare(const QImage& a, const QImage& b, int tolerance) {
  // TODO: if a.size() != b.size() set size_mismatch and return. For every pixel (x, y), pa = a.pixel(x, y) and pb = b.pixel(x, y); delta is the largest of
  // abs(qRed(pa) - qRed(pb)) and the same for green, blue and alpha. Keep the largest delta in max_delta. If delta > tolerance: ++differing, and grow bounds to
  // hold QRect(x, y, 1, 1) (the first one is the bounds, then bounds.united(...)).
  (void)a;
  (void)b;
  (void)tolerance;
  Diff d;
  d.differing = -1;
  return d;
}

// The pictures match if they are the same size and no more than `max_differing` pixels differ by more than `tolerance`.
inline bool images_match(const QImage& a, const QImage& b, int tolerance, int max_differing) {
  // TODO: compare them; true if there is no size mismatch and differing <= max_differing.
  (void)a;
  (void)b;
  (void)tolerance;
  (void)max_differing;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
