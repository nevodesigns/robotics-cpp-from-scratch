#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// A chart with several signals gives each a colour, and a label or a legend is text on a colour. Whether a person can read either depends on the contrast between
// them, which is a number and not an opinion: the ratio of the relative luminances, from 1 (the same) to 21 (black on white). Text wants at least 4.5, and a line
// at least 3.
//
// Two shortcuts fail. "Dark colour, white text" judged by the average of red, green and blue says white on pure green, and the ratio is 1.4. And a palette picked for
// how different its colours look from each other has a yellow that is a ratio of 1.3 against white: it is on the chart and nobody can see it.

struct Rgb {
  int r = 0;
  int g = 0;
  int b = 0;
};

inline bool operator==(const Rgb& a, const Rgb& b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

// The relative luminance of a colour, 0 for black to 1 for white: each channel is made linear (c / 12.92 up to 0.03928, otherwise ((c + 0.055) / 1.055)
// to the power 2.4, with c the channel over 255), and the three are weighted 0.2126, 0.7152 and 0.0722.
inline double relative_luminance(const Rgb& c) {
  // TODO: for each channel, c = value / 255.0; linear = c <= 0.03928 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4). Return 0.2126 * red + 0.7152 * green + 0.0722 * blue.
  (void)c;
  return 0.5;
}

// The contrast ratio of two colours: (lighter + 0.05) / (darker + 0.05) in relative luminance. The same either way round.
inline double contrast_ratio(const Rgb& a, const Rgb& b) {
  // TODO: la and lb are the relative luminances; the ratio is (the larger + 0.05) / (the smaller + 0.05).
  (void)a;
  (void)b;
  return 0.0;
}

// Black or white text, whichever has the higher contrast against the background. A tie is black.
inline Rgb text_on(const Rgb& background) {
  // TODO: white if contrast_ratio(background, white) is greater than contrast_ratio(background, black), otherwise black.
  (void)background;
  return Rgb{-1, -1, -1};
}

// Eight series colours that stay apart for people with the common kinds of colour blindness (the Okabe and Ito set), the same for index 0 to 7 and repeating after.
// A negative index counts back from the end.
inline Rgb series_color(int index) {
  // TODO: the palette is {0,114,178}, {213,94,0}, {0,158,115}, {204,121,167}, {230,159,0}, {86,180,233}, {240,228,66}, {0,0,0}. The index is taken
  // modulo 8, and a negative one counts back from the end: ((index % 8) + 8) % 8.
  (void)index;
  return Rgb{-1, -1, -1};
}

// The indices, 0 to 7, of the palette colours whose contrast against `background` is at least `minimum`, in order.
inline std::vector<int> usable_series(const Rgb& background, double minimum) {
  // TODO: for i from 0 to 7, keep i if contrast_ratio(series_color(i), background) >= minimum.
  (void)background;
  (void)minimum;
  return std::vector<int>(1, -1);
}

#endif  // LESSON_SOLUTION_HPP
