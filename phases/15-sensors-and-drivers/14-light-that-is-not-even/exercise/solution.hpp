#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <rc/vision/image.hpp>

using rc::vision::Gray8;

// A table that makes the sum of any rectangle of pixels cost four reads, however large the rectangle.
// Entry (x, y) holds the sum of every pixel above and to the left of it, so the table is one larger each way.
class IntegralImage {
 public:
  explicit IntegralImage(const Gray8& image) : width_(image.width > 0 ? image.width : 0), height_(image.height > 0 ? image.height : 0) {
    table_.assign(static_cast<std::size_t>(width_ + 1) * static_cast<std::size_t>(height_ + 1), 0ULL);
    // TODO: fill the table. For each row y keep a running sum of that row's pixels, and
    // entry (x + 1, y + 1) = entry (x + 1, y) + the running sum up to x. Use image.at(x, y) and the at() helper below.
  }

  int width() const { return width_; }
  int height() const { return height_; }

  // The sum of the pixels from (x0, y0) to (x1, y1) inclusive, the box clamped to the image. Zero if nothing is left.
  unsigned long long box_sum(int x0, int y0, int x1, int y1) const {
    // TODO: clamp the box with clamp(); if it is empty return 0; otherwise
    // entry(x1 + 1, y1 + 1) + entry(x0, y0) - entry(x0, y1 + 1) - entry(x1 + 1, y0).
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    return 0;
  }

  // How many pixels of that box are inside the image.
  long box_count(int x0, int y0, int x1, int y1) const {
    // TODO: clamp the box; zero if it is empty; otherwise its width times its height.
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    return 0;
  }

  // The mean over the box, using only the pixels inside the image. Zero if there are none.
  double box_mean(int x0, int y0, int x1, int y1) const {
    // TODO: box_sum over box_count, or 0 if the count is zero.
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    return 0.0;
  }

 private:
  void clamp(int& x0, int& y0, int& x1, int& y1) const {
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, width_ - 1);
    y1 = std::min(y1, height_ - 1);
  }
  unsigned long long& at(int x, int y) { return table_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_ + 1) + static_cast<std::size_t>(x)]; }
  unsigned long long at(int x, int y) const { return table_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_ + 1) + static_cast<std::size_t>(x)]; }

  int width_ = 0;
  int height_ = 0;
  std::vector<unsigned long long> table_;
};

// Mark the pixels that are darker than the neighbourhood around them. The result is width times height, row by row
// with no padding, 255 where a pixel plus `offset` is below the mean of the box of `radius` pixels either side of it, 0 elsewhere.
inline std::vector<std::uint8_t> adaptive_threshold(const Gray8& image, int radius, int offset) {
  // TODO: an empty vector for no data or no size. A negative radius is zero. Build an IntegralImage, then for every
  // pixel take box_mean(x - radius, y - radius, x + radius, y + radius) and mark it 255 if image.at(x, y) + offset < mean.
  (void)image;
  (void)radius;
  (void)offset;
  return std::vector<std::uint8_t>();
}

#endif  // LESSON_SOLUTION_HPP
