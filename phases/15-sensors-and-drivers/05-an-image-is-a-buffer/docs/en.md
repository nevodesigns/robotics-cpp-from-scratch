# An Image Is a Buffer With Opinions: Stride, Channels and Saturation

> The first row was right, which is why the test passed.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-04, 08-01, 02-01

## The Problem

A camera is a sensor like the ones in the rest of this phase: it is late, it is
noisy, and it needs calibrating. It also arrives in a shape nothing else in this
curriculum has.

A camera does not hand you a rectangle. It hands you a block of memory and four
numbers about it, and it owns the memory, often reusing it for the next frame
before you have finished with this one.

Three of those four numbers are not what you assume, and each one fails in a way
that looks like a hardware fault.

## The Concept

### The stride is not the width

Rows are padded so each begins at an address the hardware likes, usually a
multiple of four or of a cache line. A frame 13 pixels wide arrives with 16
bytes per row.

Index by width and you read three bytes further left on every row, and the error
accumulates down the image. A vertical line at column 10:

```
read with the stride the buffer has, 16     read as though the stride were 13

  ..........#..                               ..........#..
  ..........#..                               .............
  ..........#..                               #............
  ..........#..                               ...#.........
  ..........#..                               ......#......
  ..........#..                               .........#...
  ..........#..                               ............#
  ..........#..                               .............
```

| row | true column | as read |
|---|---|---|
| 0 | 10 | **10** |
| 1 | 10 | none |
| 2 | 10 | 0 |
| 3 | 10 | 3 |
| 4 | 10 | 6 |
| 5 | 10 | 9 |
| 6 | 10 | 12 |

Seven rows of eight in the wrong place, and **the first one right**.

That is the whole difficulty. A test on one row passes. A test on a frame whose
width is already a multiple of four passes, and 640 by 480 is a multiple of four.
The fault waits for a camera with an odd resolution, or a cropped region of
interest, or a format change that alters the padding.

So the stride travels with the pointer:

```cpp
struct Gray8 {
  std::uint8_t* data = nullptr;
  int width = 0, height = 0, stride = 0;
};
```

and it is read from the source every frame. `bytesPerLine`, `linesize`, `step`,
`rowPitch`: every library reports one, and it can change when a format or a
resolution does.

Two details in the indexing. **Bounds check**, because an index built from a
wrong stride runs off the end of the last row and the bytes after a frame belong
to whoever allocated next. And **cast to `std::size_t` before multiplying**,
because a 4000 by 3000 frame is twelve million pixels and `y * stride` in an
`int` overflows on a larger one, which is undefined rather than merely wrong.

### The channel order is a convention

There is no natural order for the bytes of a colour pixel, and every combination
is in use. The bytes `32, 64, 200, 255`:

| read as | red | green | blue |
|---|---|---|---|
| B, G, R, A | **200** | 64 | 32 |
| R, G, B, A | **32** | 64 | 200 |

A mostly red pixel and a mostly blue one, from the same four bytes. Both are
valid data, so nothing anywhere can object.

**Green and alpha are in the same place either way**, and so is grey. A white
balance check, a grey card, anything that looks at the green channel: all pass
under both conventions. The smallest test that can tell them apart is a pixel
that is clearly red, and it is one line.

Worth knowing: many cameras and most Windows surfaces are BGRA, most file
formats and most of the web are RGBA, OpenCV is BGR, and a lot of embedded
hardware is none of those because it packs the channels into sixteen bits.

So the order is a value that travels with the frame:

```cpp
const Colour colour = colour_at(pixel, order_of_this_camera);
```

Ask the source rather than assuming, convert once at the edge, and everything
downstream sees one convention.

### The pixel is the storage, not the arithmetic

Adding 100 to a pixel:

| before | wrapped | saturated |
|---|---|---|
| 10 | 110 | 110 |
| 100 | 200 | 200 |
| 155 | 255 | 255 |
| **156** | **0** | 255 |
| **200** | **44** | 255 |
| **255** | **99** | 255 |

Everything up to 155 is fine, which is most of a normal picture, so the fault
appears only in the bright parts: a white wall with holes in it, a lamp that is
a dark disc, a reflective marker that vanishes. It looks like a sensor problem
and it is a cast.

Downwards is stranger. Subtracting 100 from a pixel of 10 gives 166, so
darkening a picture whitens its shadows.

Neither is undefined behaviour. Conversion to an unsigned type is defined to
wrap, so no sanitizer will say a word.

```cpp
inline std::uint8_t saturating_add(std::uint8_t value, int delta) {
  const int sum = static_cast<int>(value) + delta;
  if (sum < 0) return 0;
  if (sum > 255) return 255;
  return static_cast<std::uint8_t>(sum);
}
```

Measured over a frame: 26 pixels came out darker than they went in with the
plain cast, and none with this.

The same shape catches much more than brightness. `a - b` on unsigned pixels is
enormous wherever `b` is larger. Nine pixels of 200 in a blur add to 1800, seven
times what fits. `pixel * 2` is the same trap with a different operator.

**Compute in `int` or `float`, decide at the end what to do with anything
outside the range, and cast once, on the way back.**

## Build It

Implement `at`, `set`, `saturating_add` and `colour_at` in
`exercise/solution.hpp`.

```
rcpp verify 15-05
```

The suite draws a line into a padded buffer and prints it read both ways, checks
that a view refuses to touch the padding, reads one pixel under two conventions,
and brightens a frame until part of it goes black.

## Use It

**Never hold a frame past the callback.** The buffer belongs to the driver.
Copy what you need, or take a reference the API gives you for the purpose, and
assume the memory is gone the moment you return.

**Put the width, height, stride and channel order in one struct**, filled from
the source, and pass that rather than a pointer.

**Test with a width that needs padding**, a pixel that is clearly red, and values
at 0, 1, 254 and 255. Each of the three faults here needs one specific property
in the test image, and a photograph at the native resolution has none of them.

**Convert at the edge, once.** Five conversion sites means four of them will
eventually disagree.

## What Breaks First

- **A straight line that comes back as a diagonal.** See `E-VIS-0001`.
- **A red object that comes out blue.** See `E-VIS-0002`.
- **Brightening an image until it goes black.** See `E-VIS-0003`.

## Ship It

`Gray8`, `saturating_add` and `colour_at` open `rc::vision`. The rest of this
phase's work on a camera stands on them, and none of it has to think about
padding again.
