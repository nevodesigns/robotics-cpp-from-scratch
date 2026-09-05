#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kWidth = 13;
constexpr int kHeight = 8;
constexpr int kStride = 16;   // what a camera actually hands you
constexpr int kLineAt = 10;

// A frame with a vertical line down one column, laid out the way a device lays
// one out: rows padded to a convenient boundary.
std::vector<std::uint8_t> frame_with_a_line() {
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(kStride) * kHeight, 0);
  Gray8 image{buffer.data(), kWidth, kHeight, kStride};
  for (int y = 0; y < kHeight; ++y) image.set(kLineAt, y, 255);
  return buffer;
}

void print(const char* title, const Gray8& image) {
  std::cout << "\n    " << title << "\n\n";
  for (int y = 0; y < image.height; ++y) {
    std::cout << "      ";
    for (int x = 0; x < image.width; ++x) std::cout << (image.at(x, y) ? '#' : '.');
    std::cout << "\n";
  }
}

// Which column the line appears in on each row, or -1 if the row has none.
std::vector<int> line_columns(const Gray8& image) {
  std::vector<int> columns;
  for (int y = 0; y < image.height; ++y) {
    int found = -1;
    for (int x = 0; x < image.width; ++x)
      if (image.at(x, y)) { found = x; break; }
    columns.push_back(found);
  }
  return columns;
}

}  // namespace

RC_TEST("a straight line read with the wrong stride is a diagonal") {
  std::vector<std::uint8_t> buffer = frame_with_a_line();

  const Gray8 correct{buffer.data(), kWidth, kHeight, kStride};
  const Gray8 assumed{buffer.data(), kWidth, kHeight, kWidth};   // stride == width

  print("read with the stride the buffer has, 16", correct);
  print("read as though the stride were the width, 13", assumed);

  const std::vector<int> right = line_columns(correct);
  const std::vector<int> wrong = line_columns(assumed);

  std::cout << "\n    " << std::right << std::setw(8) << "row" << std::setw(16)
            << "true column" << std::setw(14) << "as read" << "\n";
  int misplaced = 0;
  for (int y = 0; y < kHeight; ++y) {
    if (wrong[static_cast<std::size_t>(y)] != kLineAt) ++misplaced;
    std::cout << "    " << std::right << std::setw(8) << y << std::setw(16) << kLineAt
              << std::setw(14) << wrong[static_cast<std::size_t>(y)] << "\n";
  }

  std::cout << "\n    rows out of place: " << misplaced << " of " << kHeight << "\n";
  std::cout << "\n    the first row is right, which is the whole difficulty: a\n";
  std::cout << "    test on one row, or on a frame whose width happens to be a\n";
  std::cout << "    multiple of four, passes\n";

  // With the real stride the line is where it was drawn, on every row.
  for (const int column : right) RC_CHECK_EQ(column, kLineAt);

  // With the width used as the stride it walks three columns left per row.
  RC_CHECK_EQ(misplaced, kHeight - 1);
  RC_CHECK_EQ(wrong[0], kLineAt);
  RC_CHECK_EQ(wrong[2], 0);
  RC_CHECK_EQ(wrong[3], 3);
  RC_CHECK_EQ(wrong[4], 6);

  // And a frame whose rows need no padding hides all of it.
  std::vector<std::uint8_t> tidy(static_cast<std::size_t>(16) * kHeight, 0);
  Gray8 padded{tidy.data(), 16, kHeight, 16};
  for (int y = 0; y < kHeight; ++y) padded.set(kLineAt, y, 255);
  const Gray8 as_width{tidy.data(), 16, kHeight, 16};
  for (const int column : line_columns(as_width)) RC_CHECK_EQ(column, kLineAt);
}

RC_TEST("a view reads nothing outside the picture") {
  std::vector<std::uint8_t> buffer = frame_with_a_line();
  const Gray8 image{buffer.data(), kWidth, kHeight, kStride};

  RC_CHECK(image.inside(0, 0));
  RC_CHECK(image.inside(kWidth - 1, kHeight - 1));
  RC_CHECK(!image.inside(kWidth, 0));
  RC_CHECK(!image.inside(0, kHeight));
  RC_CHECK(!image.inside(-1, 0));

  // The padding is inside the buffer and outside the picture, and asking for it
  // gets nothing rather than whatever the driver left there.
  RC_CHECK_EQ(image.at(kWidth, 0), 0);
  RC_CHECK_EQ(image.at(kWidth + 2, 0), 0);
  RC_CHECK_EQ(image.at(-1, -1), 0);

  // Writing outside is refused as well, which matters more: an index from a
  // wrong stride runs off the end of the last row, and the bytes after it
  // belong to whoever allocated next.
  Gray8 writable{buffer.data(), kWidth, kHeight, kStride};
  writable.set(kWidth + 1, kHeight + 1, 255);
  writable.set(-5, 2, 255);
  RC_CHECK_EQ(buffer.back(), static_cast<std::uint8_t>(0));

  // A view over nothing answers rather than dereferencing.
  const Gray8 empty;
  RC_CHECK(!empty.inside(0, 0));
  RC_CHECK_EQ(empty.at(0, 0), 0);
}

RC_TEST("the same four bytes, read two ways") {
  // A mostly red pixel, stored the way a great many cameras and Windows
  // surfaces store one: blue first.
  const std::uint8_t pixel[4] = {32, 64, 200, 255};

  const Colour as_bgra = colour_at(pixel, ChannelOrder::bgra);
  const Colour as_rgba = colour_at(pixel, ChannelOrder::rgba);

  std::cout << "\n    the bytes 32, 64, 200, 255\n\n";
  std::cout << "    " << std::left << std::setw(20) << "read as B,G,R,A"
            << std::right << "red " << std::setw(4) << int(as_bgra.red) << "  green "
            << std::setw(4) << int(as_bgra.green) << "  blue " << std::setw(4)
            << int(as_bgra.blue) << "\n";
  std::cout << "    " << std::left << std::setw(20) << "read as R,G,B,A"
            << std::right << "red " << std::setw(4) << int(as_rgba.red) << "  green "
            << std::setw(4) << int(as_rgba.green) << "  blue " << std::setw(4)
            << int(as_rgba.blue) << "\n";

  std::cout << "\n    a mostly red pixel and a mostly blue one, from the same\n";
  std::cout << "    four bytes. Nothing reports an error, because all four\n";
  std::cout << "    bytes were valid either way\n";

  RC_CHECK_EQ(int(as_bgra.red), 200);
  RC_CHECK_EQ(int(as_bgra.blue), 32);
  RC_CHECK_EQ(int(as_rgba.red), 32);
  RC_CHECK_EQ(int(as_rgba.blue), 200);

  // Green and alpha sit in the same place either way, so a test on a grey
  // pixel, or on one that is mostly green, cannot tell the two apart.
  const std::uint8_t grey[4] = {128, 128, 128, 255};
  const Colour grey_one = colour_at(grey, ChannelOrder::bgra);
  const Colour grey_two = colour_at(grey, ChannelOrder::rgba);
  RC_CHECK_EQ(int(grey_one.red), int(grey_two.red));
  RC_CHECK_EQ(int(grey_one.blue), int(grey_two.blue));

  RC_CHECK_EQ(int(as_bgra.green), int(as_rgba.green));
  RC_CHECK_EQ(int(as_bgra.alpha), 255);

  // And nothing at all is still an answer.
  const Colour none = colour_at(nullptr, ChannelOrder::rgba);
  RC_CHECK_EQ(int(none.red), 0);
}

RC_TEST("brightening an image until it goes black") {
  std::cout << "\n    adding 100 to a pixel\n\n";
  std::cout << "    " << std::right << std::setw(10) << "before" << std::setw(14)
            << "wrapped" << std::setw(16) << "saturated" << "\n";

  for (const int value : {10, 100, 155, 156, 200, 255}) {
    const std::uint8_t wrapped = static_cast<std::uint8_t>(value + 100);
    std::cout << "    " << std::right << std::setw(10) << value << std::setw(14)
              << int(wrapped) << std::setw(16)
              << int(saturating_add(static_cast<std::uint8_t>(value), 100)) << "\n";
  }

  std::cout << "\n    156 becomes 0 and 200 becomes 44. Brightening the picture\n";
  std::cout << "    turns its brightest parts black, which looks like a sensor\n";
  std::cout << "    fault and is a cast\n";

  // The wrap, exactly.
  RC_CHECK_EQ(int(static_cast<std::uint8_t>(156 + 100)), 0);
  RC_CHECK_EQ(int(static_cast<std::uint8_t>(200 + 100)), 44);

  // And what it should have done.
  RC_CHECK_EQ(int(saturating_add(156, 100)), 255);
  RC_CHECK_EQ(int(saturating_add(200, 100)), 255);
  RC_CHECK_EQ(int(saturating_add(10, 100)), 110);

  // Downwards too, which is the same mistake in the other direction: a dark
  // pixel made darker wraps to white.
  RC_CHECK_EQ(int(static_cast<std::uint8_t>(10 - 100)), 166);
  RC_CHECK_EQ(int(saturating_add(10, -100)), 0);
  RC_CHECK_EQ(int(saturating_add(255, 0)), 255);
  RC_CHECK_EQ(int(saturating_add(0, 0)), 0);

  // A whole frame brightened, counting how many pixels came out darker than
  // they went in.
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(kStride) * kHeight, 0);
  Gray8 image{buffer.data(), kWidth, kHeight, kStride};
  for (int y = 0; y < kHeight; ++y)
    for (int x = 0; x < kWidth; ++x)
      image.set(x, y, static_cast<std::uint8_t>((y * kWidth + x) * 2));

  int darker_wrapped = 0, darker_saturated = 0;
  for (int y = 0; y < kHeight; ++y)
    for (int x = 0; x < kWidth; ++x) {
      const std::uint8_t before = image.at(x, y);
      if (static_cast<std::uint8_t>(before + 100) < before) ++darker_wrapped;
      if (saturating_add(before, 100) < before) ++darker_saturated;
    }

  std::cout << "\n    over a whole frame: " << darker_wrapped
            << " pixels came out darker with the cast, " << darker_saturated
            << " with the saturating add\n";

  RC_CHECK(darker_wrapped > 0);
  RC_CHECK_EQ(darker_saturated, 0);
}
