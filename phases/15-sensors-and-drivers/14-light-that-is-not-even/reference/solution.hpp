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
    for (int y = 0; y < height_; ++y) {
      unsigned long long row = 0;
      for (int x = 0; x < width_; ++x) {
        row += image.at(x, y);
        at(x + 1, y + 1) = at(x + 1, y) + row;
      }
    }
  }

  int width() const { return width_; }
  int height() const { return height_; }

  // The sum of the pixels from (x0, y0) to (x1, y1) inclusive, the box clamped to the image. Zero if nothing is left.
  unsigned long long box_sum(int x0, int y0, int x1, int y1) const {
    clamp(x0, y0, x1, y1);
    if (x0 > x1 || y0 > y1) return 0;
    return at(x1 + 1, y1 + 1) + at(x0, y0) - at(x0, y1 + 1) - at(x1 + 1, y0);
  }

  // How many pixels of that box are inside the image.
  long box_count(int x0, int y0, int x1, int y1) const {
    clamp(x0, y0, x1, y1);
    if (x0 > x1 || y0 > y1) return 0;
    return static_cast<long>(x1 - x0 + 1) * static_cast<long>(y1 - y0 + 1);
  }

  // The mean over the box, using only the pixels inside the image. Zero if there are none.
  double box_mean(int x0, int y0, int x1, int y1) const {
    const long n = box_count(x0, y0, x1, y1);
    if (n == 0) return 0.0;
    return static_cast<double>(box_sum(x0, y0, x1, y1)) / static_cast<double>(n);
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
  std::vector<std::uint8_t> out;
  if (image.data == nullptr || image.width <= 0 || image.height <= 0) return out;
  if (radius < 0) radius = 0;
  const IntegralImage table(image);
  out.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height), 0);
  for (int y = 0; y < image.height; ++y)
    for (int x = 0; x < image.width; ++x) {
      const double mean = table.box_mean(x - radius, y - radius, x + radius, y + radius);
      if (image.at(x, y) + offset < mean) out[static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(x)] = 255;
    }
  return out;
}

#endif  // LESSON_SOLUTION_HPP
