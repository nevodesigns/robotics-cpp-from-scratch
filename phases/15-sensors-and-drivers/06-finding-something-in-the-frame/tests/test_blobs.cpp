#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// A scene with a bright marker on a dark background, at a given exposure.
struct Scene {
  std::vector<std::uint8_t> pixels;
  int width = 0;
  int height = 0;

  Gray8 view() { return Gray8{pixels.data(), width, height, width}; }
};

constexpr int kMarkerLeft = 4;
constexpr int kMarkerTop = 2;
constexpr int kMarkerWidth = 5;
constexpr int kMarkerHeight = 4;
constexpr int kMarkerPixels = kMarkerWidth * kMarkerHeight;

Scene lit_scene(int extra_light) {
  Scene scene;
  scene.width = 20;
  scene.height = 10;
  scene.pixels.assign(static_cast<std::size_t>(scene.width) * scene.height, 0);

  for (int y = 0; y < scene.height; ++y) {
    for (int x = 0; x < scene.width; ++x) {
      const bool marker = x >= kMarkerLeft && x < kMarkerLeft + kMarkerWidth &&
                          y >= kMarkerTop && y < kMarkerTop + kMarkerHeight;
      const int value = (marker ? 200 : 40) + extra_light;
      scene.pixels[static_cast<std::size_t>(y) * scene.width + x] =
          rc::vision::saturating_add(0, value);
    }
  }
  return scene;
}

void show(const char* title, const Gray8& image, int threshold) {
  std::cout << "\n    " << title << "\n\n";
  for (int y = 0; y < image.height; ++y) {
    std::cout << "      ";
    for (int x = 0; x < image.width; ++x)
      std::cout << (image.at(x, y) > threshold ? '#' : '.');
    std::cout << "\n";
  }
}

}  // namespace

RC_TEST("a histogram is what an automatic threshold has to work with") {
  Scene scene = lit_scene(0);
  const Gray8 image = scene.view();
  const auto counts = histogram_of(image);

  long total = 0;
  for (const long value : counts) total += value;
  RC_CHECK_EQ(total, static_cast<long>(image.width) * image.height);

  // Two values in this picture and nothing else, which is what makes it a good
  // first test and a bad photograph.
  RC_CHECK_EQ(counts[200], static_cast<long>(kMarkerPixels));
  RC_CHECK_EQ(counts[40], static_cast<long>(image.width * image.height - kMarkerPixels));
  RC_CHECK_EQ(counts[128], 0L);

  // An empty image has a histogram and no threshold worth having.
  const Gray8 nothing;
  const auto empty = histogram_of(nothing);
  RC_CHECK_EQ(empty[0], 0L);
  RC_CHECK_EQ(otsu_threshold(empty), 0);
}

RC_TEST("a fixed threshold is a statement about the lighting") {
  std::cout << "\n    a marker at 200 on a background at 40, as the lights come up\n\n";
  std::cout << "    " << std::right << std::setw(10) << "extra" << std::setw(12)
            << "background" << std::setw(10) << "marker" << std::setw(16)
            << "fixed 128" << std::setw(16) << "otsu / found" << "\n";

  int fixed_at_worst = 0;
  for (const int extra : {0, 40, 80, 120}) {
    Scene scene = lit_scene(extra);
    const Gray8 image = scene.view();

    int fixed_hits = 0;
    for (int y = 0; y < image.height; ++y)
      for (int x = 0; x < image.width; ++x)
        if (image.at(x, y) > 128) ++fixed_hits;

    const int threshold = otsu_threshold(histogram_of(image));
    int otsu_hits = 0;
    for (int y = 0; y < image.height; ++y)
      for (int x = 0; x < image.width; ++x)
        if (image.at(x, y) > threshold) ++otsu_hits;

    if (extra == 120) fixed_at_worst = fixed_hits;

    std::cout << "    " << std::right << std::setw(10) << extra << std::setw(12)
              << int(image.at(0, 0)) << std::setw(10)
              << int(image.at(kMarkerLeft, kMarkerTop)) << std::setw(16) << fixed_hits
              << std::setw(10) << threshold << " /" << std::setw(4) << otsu_hits << "\n";

    // Otsu finds the marker, and only the marker, at every exposure.
    RC_CHECK_EQ(otsu_hits, kMarkerPixels);
  }

  std::cout << "\n    the marker is " << kMarkerPixels << " pixels. At the last row\n";
  std::cout << "    the background itself is above 128, so the fixed threshold\n";
  std::cout << "    selects the whole frame\n";

  // The whole image, all 200 pixels of it.
  RC_CHECK_EQ(fixed_at_worst, 200);

  // And it worked perfectly for the first three rows, which is how it ships.
  Scene ordinary = lit_scene(0);
  const Gray8 image = ordinary.view();
  int hits = 0;
  for (int y = 0; y < image.height; ++y)
    for (int x = 0; x < image.width; ++x)
      if (image.at(x, y) > 128) ++hits;
  RC_CHECK_EQ(hits, kMarkerPixels);
}

RC_TEST("what counts as one thing is a decision, not a fact") {
  // A diagonal chain of five pixels, touching only at their corners.
  std::vector<std::uint8_t> pixels(7 * 7, 0);
  Gray8 image{pixels.data(), 7, 7, 7};
  for (int i = 0; i < 5; ++i) image.set(i + 1, i + 1, 255);

  show("a diagonal chain of five pixels", image, 128);

  const auto four = find_blobs(image, 128, Connectivity::four);
  const auto eight = find_blobs(image, 128, Connectivity::eight);

  std::cout << "\n    four connected:  " << four.size() << " blobs\n";
  std::cout << "    eight connected: " << eight.size() << " blobs\n";
  std::cout << "\n    nothing about the picture says which is meant. A crack in\n";
  std::cout << "    a surface wants one answer and two markers that happen to\n";
  std::cout << "    touch at a corner want the other\n";

  RC_CHECK_EQ(static_cast<int>(four.size()), 5);
  RC_CHECK_EQ(static_cast<int>(eight.size()), 1);

  // Every pixel is accounted for either way, which is the one thing both
  // answers have to agree on.
  int four_total = 0;
  for (const Blob& blob : four) four_total += blob.pixels;
  RC_CHECK_EQ(four_total, 5);
  RC_CHECK_EQ(eight.front().pixels, 5);

  // A solid shape is one blob under both, which is why a test on a square
  // cannot tell them apart.
  Scene scene = lit_scene(0);
  const Gray8 solid = scene.view();
  RC_CHECK_EQ(static_cast<int>(find_blobs(solid, 128, Connectivity::four).size()), 1);
  RC_CHECK_EQ(static_cast<int>(find_blobs(solid, 128, Connectivity::eight).size()), 1);
}

RC_TEST("the centre of mass is not the centre of the box") {
  std::vector<std::uint8_t> pixels(8 * 8, 0);
  Gray8 image{pixels.data(), 8, 8, 8};
  for (int y = 1; y < 7; ++y) image.set(1, y, 255);   // the upright
  for (int x = 1; x < 7; ++x) image.set(x, 6, 255);   // the foot

  show("an L shape", image, 128);

  const auto blobs = find_blobs(image, 128, Connectivity::eight);
  RC_REQUIRE_EQ(static_cast<int>(blobs.size()), 1);
  const Blob& blob = blobs.front();

  const double box_x = (blob.left + blob.right) / 2.0;
  const double box_y = (blob.top + blob.bottom) / 2.0;
  const double apart = std::hypot(blob.centre_x - box_x, blob.centre_y - box_y);

  std::cout << "\n    " << std::left << std::setw(24) << "pixels" << std::right
            << blob.pixels << "\n";
  std::cout << "    " << std::left << std::setw(24) << "centre of mass"
            << std::right << std::fixed << std::setprecision(2) << blob.centre_x
            << ", " << blob.centre_y << "\n";
  std::cout << "    " << std::left << std::setw(24) << "centre of the box"
            << std::right << box_x << ", " << box_y << "\n";
  std::cout << "    " << std::left << std::setw(24) << "apart" << std::right
            << apart << " pixels\n";

  std::cout << "\n    a quarter of the object, on a shape six pixels tall. The\n";
  std::cout << "    centroid is where the thing is and the box centre is where\n";
  std::cout << "    the space it occupies is, and they agree only for a\n";
  std::cout << "    symmetric shape\n";

  RC_CHECK_EQ(blob.pixels, 11);
  RC_CHECK_NEAR(blob.centre_x, 2.36, 0.01);
  RC_CHECK_NEAR(blob.centre_y, 4.64, 0.01);
  RC_CHECK_NEAR(box_x, 3.5, 1e-12);
  RC_CHECK_NEAR(box_y, 3.5, 1e-12);
  RC_CHECK_NEAR(apart, 1.61, 0.01);

  // On a rectangle they are the same point, which is what a test on a square
  // marker would have shown and why one is not enough.
  Scene scene = lit_scene(0);
  const auto square = find_blobs(scene.view(), 128, Connectivity::eight);
  RC_REQUIRE_EQ(static_cast<int>(square.size()), 1);
  RC_CHECK_NEAR(square.front().centre_x,
                (square.front().left + square.front().right) / 2.0, 1e-12);
  RC_CHECK_NEAR(square.front().centre_y,
                (square.front().top + square.front().bottom) / 2.0, 1e-12);
}

RC_TEST("a threshold from the picture, on a picture with nothing in it") {
  // An evenly lit wall. There is no marker, and Otsu will still return a
  // threshold, because returning a threshold is all it does.
  std::vector<std::uint8_t> pixels(20 * 10, 90);
  const Gray8 flat{pixels.data(), 20, 10, 20};

  const int threshold = otsu_threshold(histogram_of(flat));
  const auto blobs = find_blobs(flat, threshold, Connectivity::eight);

  std::cout << "\n    an evenly lit wall, every pixel 90\n\n";
  std::cout << "    " << std::left << std::setw(22) << "otsu says" << std::right
            << threshold << "\n";
  std::cout << "    " << std::left << std::setw(22) << "blobs found" << std::right
            << blobs.size() << "\n";
  std::cout << "    " << std::left << std::setw(22) << "pixels in it" << std::right
            << (blobs.empty() ? 0 : blobs.front().pixels) << " of "
            << flat.width * flat.height << "\n";

  std::cout << "\n    not nothing: everything. With no two groups to separate,\n";
  std::cout << "    the best split is at the bottom and every pixel is above\n";
  std::cout << "    it, so the marker the robot goes to is the whole frame and\n";
  std::cout << "    its centre is the middle of the picture\n";

  // The failure is that it finds everything, which is worse than finding
  // nothing: a caller checking "did we see a marker" is told yes.
  RC_REQUIRE_EQ(static_cast<int>(blobs.size()), 1);
  RC_CHECK_EQ(blobs.front().pixels, flat.width * flat.height);
  RC_CHECK_NEAR(blobs.front().centre_x, (flat.width - 1) / 2.0, 1e-9);

  // So an automatic threshold answers where to split and not whether there is
  // anything to split, and something after it has to decide that. The cheapest
  // test is how much of the frame came back.
  const double fraction =
      static_cast<double>(blobs.front().pixels) / (flat.width * flat.height);
  RC_CHECK(fraction > 0.5);

  // On a picture that does have two groups in it, the same check passes easily.
  Scene scene = lit_scene(0);
  const Gray8 real = scene.view();
  const auto found = find_blobs(real, otsu_threshold(histogram_of(real)),
                                Connectivity::eight);
  RC_REQUIRE_EQ(static_cast<int>(found.size()), 1);
  RC_CHECK(static_cast<double>(found.front().pixels) / (real.width * real.height) < 0.2);

  // And the other end of the same problem: one hot pixel is a blob of one.
  std::vector<std::uint8_t> speckled(20 * 10, 90);
  Gray8 with_speck{speckled.data(), 20, 10, 20};
  with_speck.set(7, 3, 255);
  const auto specks = find_blobs(with_speck, otsu_threshold(histogram_of(with_speck)),
                                 Connectivity::eight);
  RC_REQUIRE_EQ(static_cast<int>(specks.size()), 1);
  RC_CHECK_EQ(specks.front().pixels, 1);

  std::cout << "\n    at the other end, one hot pixel is a blob of one, so a\n";
  std::cout << "    size range is not an optimisation. It is part of saying\n";
  std::cout << "    what you are looking for\n";
}
