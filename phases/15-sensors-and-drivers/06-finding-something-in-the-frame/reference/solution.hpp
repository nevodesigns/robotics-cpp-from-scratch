#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <rc/vision/image.hpp>

using rc::vision::Gray8;

// How many pixels of the image have each value.
//
// The whole of what an automatic threshold has to work with, and 256 numbers
// rather than a million.
inline std::array<long, 256> histogram_of(const Gray8& image) {
  std::array<long, 256> counts{};
  for (int y = 0; y < image.height; ++y)
    for (int x = 0; x < image.width; ++x) ++counts[image.at(x, y)];
  return counts;
}

// The threshold that best separates the picture into two groups, by Otsu's
// method: the one that puts the most variance between the groups.
//
// A constant threshold is a statement about the lighting, and lighting is the
// one thing about a scene that is guaranteed to change. Measured, a marker at
// 200 against a background at 40: a fixed threshold of 128 finds it correctly
// until the lights come up by 120, at which point the background is at 160 and
// the threshold selects the entire image. Otsu finds exactly the marker at
// every level.
//
// It is one pass over 256 numbers, which costs nothing next to reading the
// frame.
inline int otsu_threshold(const std::array<long, 256>& counts) {
  long total = 0;
  double weighted = 0.0;
  for (int value = 0; value < 256; ++value) {
    total += counts[static_cast<std::size_t>(value)];
    weighted += value * static_cast<double>(counts[static_cast<std::size_t>(value)]);
  }
  if (total == 0) return 0;

  long below_count = 0;
  double below_sum = 0.0;
  double best_spread = -1.0;
  int best = 0;

  for (int t = 0; t < 256; ++t) {
    below_count += counts[static_cast<std::size_t>(t)];
    if (below_count == 0) continue;
    const long above_count = total - below_count;
    if (above_count == 0) break;

    below_sum += t * static_cast<double>(counts[static_cast<std::size_t>(t)]);
    const double below_mean = below_sum / static_cast<double>(below_count);
    const double above_mean = (weighted - below_sum) / static_cast<double>(above_count);
    const double gap = below_mean - above_mean;
    const double spread =
        static_cast<double>(below_count) * static_cast<double>(above_count) * gap * gap;

    if (spread > best_spread) {
      best_spread = spread;
      best = t;
    }
  }
  return best;
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

// Every connected region brighter than the threshold.
inline std::vector<Blob> find_blobs(const Gray8& image, int threshold,
                                    Connectivity connectivity) {
  std::vector<Blob> blobs;
  if (image.data == nullptr || image.width <= 0 || image.height <= 0) return blobs;

  const std::size_t count =
      static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
  std::vector<bool> seen(count, false);
  std::vector<int> stack;

  const auto index_of = [&image](int x, int y) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
           static_cast<std::size_t>(x);
  };

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      if (seen[index_of(x, y)]) continue;
      if (image.at(x, y) <= threshold) continue;

      Blob blob;
      blob.left = blob.right = x;
      blob.top = blob.bottom = y;
      double sum_x = 0.0, sum_y = 0.0;

      // A stack rather than recursion. A blob can be the whole frame, and a
      // million deep recursion is a stack overflow rather than a slow answer.
      seen[index_of(x, y)] = true;
      stack.push_back(static_cast<int>(index_of(x, y)));

      while (!stack.empty()) {
        const int here = stack.back();
        stack.pop_back();
        const int hx = here % image.width;
        const int hy = here / image.width;

        ++blob.pixels;
        sum_x += hx;
        sum_y += hy;
        if (hx < blob.left) blob.left = hx;
        if (hx > blob.right) blob.right = hx;
        if (hy < blob.top) blob.top = hy;
        if (hy > blob.bottom) blob.bottom = hy;

        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            if (connectivity == Connectivity::four && dx != 0 && dy != 0) continue;

            const int nx = hx + dx, ny = hy + dy;
            if (!image.inside(nx, ny)) continue;
            if (seen[index_of(nx, ny)]) continue;
            if (image.at(nx, ny) <= threshold) continue;

            seen[index_of(nx, ny)] = true;
            stack.push_back(static_cast<int>(index_of(nx, ny)));
          }
        }
      }

      blob.centre_x = sum_x / blob.pixels;
      blob.centre_y = sum_y / blob.pixels;
      blobs.push_back(blob);
    }
  }
  return blobs;
}

#endif  // LESSON_SOLUTION_HPP
