#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>

// A view of somebody else's pixels.
//
// A camera does not hand you a rectangle, it hands you a block of memory and
// four numbers about it, and three of the four are usually not what you assume.
//
// It owns nothing. The buffer belongs to the driver, the frame grabber, or the
// library that decoded the file, and it is often reused for the next frame
// before you have finished with this one.
struct Gray8 {
  std::uint8_t* data = nullptr;
  int width = 0;
  int height = 0;

  // The distance from the start of one row to the start of the next, which is
  // not the width.
  //
  // Rows are padded so that each begins at a convenient address, so a 13 pixel
  // wide frame usually arrives with 16 bytes per row. Indexing by width instead
  // reads three bytes further left on every row, so a vertical line comes back
  // as a diagonal and seven rows out of eight are in the wrong place.
  int stride = 0;

  bool inside(int x, int y) const {
    return data != nullptr && x >= 0 && y >= 0 && x < width && y < height;
  }

  // TODO 1: the pixel at (x, y), and setting one.
  //
  // The byte is at y * stride + x, not y * width + x. That one substitution is
  // the whole of this lesson's first table: a vertical line read with the width
  // in place of the stride comes back as a diagonal, with seven rows out of
  // eight in the wrong place and the first row correct.
  //
  // Read nothing and write nothing outside the picture. Returning zero rather
  // than reading anyway matters because an index computed from a wrong stride
  // runs off the end of the last row, and the bytes beyond it belong to
  // whoever allocated after the frame.
  //
  // Cast to std::size_t before multiplying. A 4000 by 3000 frame is twelve
  // million pixels and y * stride overflows an int long before that on a large
  // one.
  std::uint8_t at(int x, int y) const {
    (void)x;
    (void)y;
    return 0;
  }

  void set(int x, int y, std::uint8_t value) {
    (void)x;
    (void)y;
    (void)value;
  }
};

// TODO 2: add to a pixel without wrapping round.
//
// Do the arithmetic in an int, then clamp to 0 and 255 before narrowing back.
//
// A pixel is eight bits and the arithmetic on it is not. Brightening an image by
// adding 100 in the obvious way turns 156 into 0 and 200 into 44: the brightest
// parts of the picture come out black, which looks like a sensor fault and is a
// cast. The same mistake downwards turns a dark pixel white.
inline std::uint8_t saturating_add(std::uint8_t value, int delta) {
  (void)delta;
  return value;
}

// Which byte of a colour pixel is which.
//
// There is no natural order and every combination is in use. Reading the four
// bytes of a mostly red pixel in the wrong order gives a mostly blue one, and
// nothing anywhere reports an error, because all four bytes were valid.
enum class ChannelOrder {
  rgba,
  bgra,
};

struct Colour {
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;
  std::uint8_t alpha = 255;
};

// TODO 3: pull a colour out of four bytes, in the order they are actually in.
//
// rgba is red, green, blue, alpha. bgra is blue, green, red, alpha. Alpha is
// last either way and green is in the middle either way, which is exactly why a
// test on a grey pixel cannot tell the two apart.
//
// Return a default colour for a null pointer rather than reading through it.
inline Colour colour_at(const std::uint8_t* pixel, ChannelOrder order) {
  (void)pixel;
  (void)order;
  return Colour{};
}

#endif  // LESSON_SOLUTION_HPP
