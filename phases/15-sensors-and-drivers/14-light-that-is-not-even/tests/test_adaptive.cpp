#include <rc/test/rc_test.hpp>
#include <rc/vision/blobs.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

const int kW = 160;
const int kH = 120;
const int kStride = 163;   // rows are padded, as a camera pads them

// A scene lit from the left: the background goes from 40 to 220 across the frame, and three dark marks 6 pixels thick
// sit on it, 45 grey levels darker than the background around them.
struct Scene {
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint8_t> truth;   // 255 on a mark
  Gray8 image;

  explicit Scene(double noise_sigma, std::uint32_t seed = 4) {
    bytes.assign(static_cast<std::size_t>(kStride) * kH, 0);
    truth.assign(static_cast<std::size_t>(kW) * kH, 0);
    Rng rng{seed};
    for (int y = 0; y < kH; ++y)
      for (int x = 0; x < kW; ++x) {
        double v = 40.0 + 180.0 * x / (kW - 1);
        const bool mark = (x >= 20 && x < 26) || (x >= 130 && x < 136) || (y >= 55 && y < 61);
        if (mark) v -= 45.0;
        v += noise_sigma * rng.gaussian();
        if (v < 0.0) v = 0.0;
        if (v > 255.0) v = 255.0;
        bytes[static_cast<std::size_t>(y) * kStride + x] = static_cast<std::uint8_t>(v + 0.5);
        if (mark) truth[static_cast<std::size_t>(y) * kW + x] = 255;
      }
    image.data = bytes.data();
    image.width = kW;
    image.height = kH;
    image.stride = kStride;
  }
};

struct Score {
  double found = 0.0;     // share of the mark pixels that were marked
  double spurious = 0.0;  // share of the other pixels that were marked
};

Score score(const std::vector<std::uint8_t>& mask, const std::vector<std::uint8_t>& truth) {
  long marks = 0;
  long found = 0;
  long others = 0;
  long spurious = 0;
  for (std::size_t i = 0; i < truth.size() && i < mask.size(); ++i) {
    if (truth[i]) {
      ++marks;
      if (mask[i]) ++found;
    } else {
      ++others;
      if (mask[i]) ++spurious;
    }
  }
  Score s;
  s.found = marks ? static_cast<double>(found) / marks : 0.0;
  s.spurious = others ? static_cast<double>(spurious) / others : 0.0;
  return s;
}

}  // namespace

RC_TEST("the sum of any rectangle from the table is the sum of its pixels, at the edges too") {
  Scene scene(5.0);
  const IntegralImage table(scene.image);
  RC_CHECK(table.width() == kW);
  RC_CHECK(table.height() == kH);

  Rng rng{9};
  long wrong = 0;
  const int trials = 2000;
  for (int i = 0; i < trials; ++i) {
    const int x0 = static_cast<int>(rng.next() % (kW + 20)) - 10;
    const int y0 = static_cast<int>(rng.next() % (kH + 20)) - 10;
    const int x1 = x0 + static_cast<int>(rng.next() % 60);
    const int y1 = y0 + static_cast<int>(rng.next() % 60);
    unsigned long long brute = 0;
    long count = 0;
    for (int y = y0; y <= y1; ++y)
      for (int x = x0; x <= x1; ++x)
        if (scene.image.inside(x, y)) {
          brute += scene.image.at(x, y);
          ++count;
        }
    if (table.box_sum(x0, y0, x1, y1) != brute) ++wrong;
    if (table.box_count(x0, y0, x1, y1) != count) ++wrong;
  }
  std::cout << "  " << trials << " random boxes, some partly or wholly outside the picture, against a pixel by pixel sum: " << wrong
            << " wrong\n\n";
  RC_CHECK(wrong == 0);
  RC_CHECK(table.box_sum(0, 0, kW - 1, kH - 1) > 0);
  RC_CHECK(table.box_sum(-50, -50, -10, -10) == 0);
  RC_CHECK_NEAR(table.box_mean(-50, -50, -10, -10), 0.0, 1e-12);
}

RC_TEST("a mean near the edge uses only the pixels that exist, so a flat picture has the same mean everywhere") {
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(kStride) * kH, 0);
  for (int y = 0; y < kH; ++y)
    for (int x = 0; x < kW; ++x) bytes[static_cast<std::size_t>(y) * kStride + x] = 90;
  Gray8 flat;
  flat.data = bytes.data();
  flat.width = kW;
  flat.height = kH;
  flat.stride = kStride;
  const IntegralImage table(flat);
  RC_CHECK_NEAR(table.box_mean(0 - 8, 0 - 8, 0 + 8, 0 + 8), 90.0, 1e-9);                 // the corner
  RC_CHECK_NEAR(table.box_mean(kW - 1 - 8, kH - 1 - 8, kW - 1 + 8, kH - 1 + 8), 90.0, 1e-9);
  RC_CHECK_NEAR(table.box_mean(70, 50, 80, 60), 90.0, 1e-9);
  RC_CHECK(table.box_count(-8, -8, 8, 8) == 81);   // 9 by 9: most of the 17 by 17 box is outside
  std::cout << "  a box of 17 by 17 at the corner holds " << table.box_count(-8, -8, 8, 8) << " pixels of the picture, mean "
            << std::fixed << std::setprecision(1) << table.box_mean(-8, -8, 8, 8) << "\n\n";

  const std::vector<std::uint8_t> marked = adaptive_threshold(flat, 8, 5);
  RC_REQUIRE(marked.size() == static_cast<std::size_t>(kW) * kH);
  long any = 0;
  for (std::uint8_t m : marked) any += m ? 1 : 0;
  RC_CHECK(any == 0);
}

RC_TEST("one threshold for a picture lit from one side fails, and a local one finds the marks") {
  Scene scene(0.0);
  const auto counts = rc::vision::histogram_of(scene.image);
  const int t = rc::vision::otsu_threshold(counts);
  std::vector<std::uint8_t> global(static_cast<std::size_t>(kW) * kH, 0);
  for (int y = 0; y < kH; ++y)
    for (int x = 0; x < kW; ++x)
      if (scene.image.at(x, y) <= t) global[static_cast<std::size_t>(y) * kW + x] = 255;
  const Score g = score(global, scene.truth);
  const Score a = score(adaptive_threshold(scene.image, 12, 10), scene.truth);
  std::cout << "  scene lit from the left, marks 45 levels darker than their surroundings\n"
            << "  method               marks found   other pixels marked\n"
            << "  one threshold (" << std::setw(3) << t << ")    " << std::fixed << std::setprecision(3) << std::setw(8) << g.found
            << "      " << std::setw(8) << g.spurious << "\n"
            << "  local, radius 12     " << std::setw(8) << a.found << "      " << std::setw(8) << a.spurious << "\n\n";
  RC_CHECK(a.found > 0.0);
  RC_CHECK(g.spurious > 0.3);
  RC_CHECK(a.found > 0.8);
  RC_CHECK(a.spurious < 0.02);
}

RC_TEST("a window smaller than a mark eats it, and a window too large is a global threshold again") {
  Scene scene(0.0);
  std::cout << "  radius   marks found   other pixels marked\n";
  double found_small = 0.0;
  double found_good = 0.0;
  double spurious_good = 0.0;
  double spurious_huge = 0.0;
  for (int radius : {1, 2, 4, 8, 16, 40, 100}) {
    const Score s = score(adaptive_threshold(scene.image, radius, 10), scene.truth);
    std::cout << "  " << std::setw(6) << radius << "   " << std::fixed << std::setprecision(3) << std::setw(11) << s.found
              << "   " << std::setw(19) << s.spurious << "\n";
    if (radius == 1) found_small = s.found;
    if (radius == 16) {
      found_good = s.found;
      spurious_good = s.spurious;
    }
    if (radius == 100) spurious_huge = s.spurious;
  }
  std::cout << "\n";
  RC_CHECK(found_good > 0.8);
  RC_CHECK(found_small < 0.5 * found_good);
  RC_CHECK(spurious_huge > 5.0 * spurious_good + 0.01);
}

RC_TEST("with noise in the picture the offset is what keeps the background from being marked") {
  Scene scene(6.0);
  std::cout << "  noise of 6 grey levels\n  offset   marks found   other pixels marked\n";
  double spurious_zero = 0.0;
  double spurious_ten = 0.0;
  double found_ten = 0.0;
  double found_thirty = 0.0;
  for (int offset : {0, 4, 8, 10, 14, 20, 30}) {
    const Score s = score(adaptive_threshold(scene.image, 16, offset), scene.truth);
    std::cout << "  " << std::setw(6) << offset << "   " << std::fixed << std::setprecision(3) << std::setw(11) << s.found << "   "
              << std::setw(19) << s.spurious << "\n";
    if (offset == 0) spurious_zero = s.spurious;
    if (offset == 10) {
      spurious_ten = s.spurious;
      found_ten = s.found;
    }
    if (offset == 30) found_thirty = s.found;
  }
  std::cout << "\n";
  RC_CHECK(spurious_zero > 0.2);
  RC_CHECK(spurious_ten < 0.2 * spurious_zero);
  RC_CHECK(found_ten > 0.5);
  RC_CHECK(found_thirty < found_ten);
}
