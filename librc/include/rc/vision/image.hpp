// rc/vision/image.hpp
//
// A view of somebody else's pixels, from lesson 15-05, and the three things
// about them that are never what you assume.
//
// A camera does not hand you a rectangle. It hands you a block of memory and
// four numbers about it, and it owns the memory, often reusing it for the next
// frame before you have finished with this one.
//
// The stride is not the width. Rows are padded so each begins at a convenient
// address, so a 13 pixel wide frame arrives with 16 bytes per row. Measured, a
// vertical line read with the width in place of the stride walks three columns
// left per row: seven rows of eight in the wrong place, and the first row
// correct, which is why a one row test passes.
//
// The channel order is a guess. The bytes 32, 64, 200, 255 are a mostly red
// pixel read one way and a mostly blue one read the other, and nothing reports
// an error because all four bytes were valid either way. Green and alpha sit in
// the same place in both, so a grey pixel cannot tell them apart.
//
// The arithmetic is not eight bits. Adding 100 to a pixel turns 156 into 0 and
// 200 into 44, so brightening a picture blackens its brightest parts.

#ifndef RC_VISION_IMAGE
#define RC_VISION_IMAGE

#include <cstddef>
#include <cstdint>

namespace rc {
namespace vision {

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

  // The pixel at (x, y), or zero outside the image.
  //
  // Returning zero rather than reading anyway: an index computed from the wrong
  // stride runs off the end of the last row, and the bytes beyond it belong to
  // whoever allocated after the frame.
  std::uint8_t at(int x, int y) const {
    if (!inside(x, y)) return 0;
    return data[static_cast<std::size_t>(y) * static_cast<std::size_t>(stride) +
                static_cast<std::size_t>(x)];
  }

  void set(int x, int y, std::uint8_t value) {
    if (!inside(x, y)) return;
    data[static_cast<std::size_t>(y) * static_cast<std::size_t>(stride) +
         static_cast<std::size_t>(x)] = value;
  }
};

// Add to a pixel without wrapping round.
//
// A pixel is eight bits and the arithmetic on it is not. Brightening an image
// by adding 100 to every pixel, in the obvious way, turns 156 into 0 and 200
// into 44: the brightest parts of the picture come out black, which looks like
// a sensor fault and is a cast.
inline std::uint8_t saturating_add(std::uint8_t value, int delta) {
  const int sum = static_cast<int>(value) + delta;
  if (sum < 0) return 0;
  if (sum > 255) return 255;
  return static_cast<std::uint8_t>(sum);
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

inline Colour colour_at(const std::uint8_t* pixel, ChannelOrder order) {
  Colour colour;
  if (pixel == nullptr) return colour;

  if (order == ChannelOrder::rgba) {
    colour.red = pixel[0];
    colour.green = pixel[1];
    colour.blue = pixel[2];
  } else {
    colour.blue = pixel[0];
    colour.green = pixel[1];
    colour.red = pixel[2];
  }
  colour.alpha = pixel[3];
  return colour;
}

}  // namespace vision
}  // namespace rc

#endif  // RC_VISION_IMAGE
