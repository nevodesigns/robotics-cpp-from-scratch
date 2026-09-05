#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <rc/vision/image.hpp>

using rc::vision::Gray8;

// TODO 1: how many pixels of the image have each value.
//
// One pass over the picture, counting into 256 buckets. That is the whole of
// what an automatic threshold has to work with, and it is 256 numbers rather
// than a million.
inline std::array<long, 256> histogram_of(const Gray8& image) {
  (void)image;
  return std::array<long, 256>{};
}

// TODO 2: the threshold that best separates the picture into two groups.
//
// Otsu's method: try every threshold and keep the one that puts the most
// variance between the two groups it makes.
//
// Walk t from 0 to 255, keeping a running count and a running weighted sum of
// everything at or below t. At each step the two group sizes and their two
// means are known, and the spread to maximise is
//
//     below_count * above_count * (below_mean - above_mean)^2
//
// Skip a t where either group is empty, and return 0 for an empty image.
//
// A constant threshold is a statement about the lighting, and lighting is the
// one thing about a scene guaranteed to change. Measured, a marker at 200 on a
// background at 40: a fixed 128 works until the lights come up by 120, at which
// point the background is 160 and the threshold selects the entire frame. This
// finds exactly the marker at every level, in one pass over 256 numbers.
inline int otsu_threshold(const std::array<long, 256>& counts) {
  (void)counts;
  return 128;
}

// One connected region of bright pixels.
struct Blob {
  int pixels = 0;

  // The centre of mass, which is not the centre of the bounding box.
  //
  // On an L shape six pixels tall the two are 1.61 pixels apart, which is a
  // quarter of the object. Which one you want depends on the question: the
  // centroid is where the thing is, the box centre is where the space it
  // occupies is, and they agree only for a symmetric shape.
  double centre_x = 0.0;
  double centre_y = 0.0;

  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
};

// Whether two pixels touching only at a corner are the same thing.
//
// There is no right answer, only a decision. A diagonal chain of five pixels is
// five separate blobs under four connectivity and one blob under eight, and
// nothing about the picture says which is meant.
enum class Connectivity {
  four,
  eight,
};

// TODO 3: every connected region brighter than the threshold.
//
// Walk the picture. At each unvisited pixel above the threshold, start a new
// blob and grow it: keep a stack of pixels to look at, and for each one, push
// every unvisited neighbour that is also above the threshold.
//
// Use a stack rather than recursion. A blob can be the whole frame, and a
// million deep recursion is a stack overflow rather than a slow answer.
//
// Which pixels count as neighbours is the `connectivity` argument: four means
// the ones sharing an edge, eight includes the corners. Skip the diagonals when
// four is asked for.
//
// While growing, accumulate the pixel count, the running sums of x and y for
// the centre of mass, and the four edges of the bounding box. Divide the sums
// by the count at the end.
//
// The centre of mass is not the centre of the box. On an L six pixels tall the
// two are 1.61 pixels apart, and they agree only for a symmetric shape.
inline std::vector<Blob> find_blobs(const Gray8& image, int threshold,
                                    Connectivity connectivity) {
  (void)image;
  (void)threshold;
  (void)connectivity;
  return std::vector<Blob>{};
}

#endif  // LESSON_SOLUTION_HPP
