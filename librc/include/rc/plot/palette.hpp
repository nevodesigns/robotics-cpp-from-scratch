// rc/plot/palette.hpp
//
// Contrast, readable text and usable series colours, from lesson 10-13, graduated.
//
// Black on white is 21 and a colour on itself is 1; white text on pure green is 1.37 where black is 15.3, and the average-brightness rule picks the white; of the eight Okabe and Ito colours only five (black among them) reach a ratio of 3 on a white
// background, and the yellow is 1.32.

#ifndef RC_PLOT_PALETTE_HPP
#define RC_PLOT_PALETTE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace plot {

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
  const double channel[3] = {c.r / 255.0, c.g / 255.0, c.b / 255.0};
  double linear[3];
  for (int i = 0; i < 3; ++i) linear[i] = channel[i] <= 0.03928 ? channel[i] / 12.92 : std::pow((channel[i] + 0.055) / 1.055, 2.4);
  return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2];
}

// The contrast ratio of two colours: (lighter + 0.05) / (darker + 0.05) in relative luminance. The same either way round.
inline double contrast_ratio(const Rgb& a, const Rgb& b) {
  const double la = relative_luminance(a);
  const double lb = relative_luminance(b);
  const double lighter = la > lb ? la : lb;
  const double darker = la > lb ? lb : la;
  return (lighter + 0.05) / (darker + 0.05);
}

// Black or white text, whichever has the higher contrast against the background. A tie is black.
inline Rgb text_on(const Rgb& background) {
  const Rgb black{0, 0, 0};
  const Rgb white{255, 255, 255};
  return contrast_ratio(background, white) > contrast_ratio(background, black) ? white : black;
}

// Eight series colours that stay apart for people with the common kinds of colour blindness (the Okabe and Ito set), the same for index 0 to 7 and repeating after.
// A negative index counts back from the end.
inline Rgb series_color(int index) {
  static const Rgb palette[8] = {{0, 114, 178}, {213, 94, 0}, {0, 158, 115}, {204, 121, 167}, {230, 159, 0}, {86, 180, 233}, {240, 228, 66}, {0, 0, 0}};
  const int i = ((index % 8) + 8) % 8;
  return palette[i];
}

// The indices, 0 to 7, of the palette colours whose contrast against `background` is at least `minimum`, in order.
inline std::vector<int> usable_series(const Rgb& background, double minimum) {
  std::vector<int> out;
  for (int i = 0; i < 8; ++i)
    if (contrast_ratio(series_color(i), background) >= minimum) out.push_back(i);
  return out;
}
}  // namespace plot
}  // namespace rc

#endif  // RC_PLOT_PALETTE_HPP
